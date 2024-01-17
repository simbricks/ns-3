/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Copyright 2020 Max Planck Institute for Software Systems
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
 * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#include "simbricks-initmgr.h"

#include "ns3/log.h"
#include "ns3/simulator.h"
#include "ns3/nstime.h"

#include <iostream>
#include <csignal>
#include <poll.h>


namespace ns3 {
namespace simbricks {
namespace base {

NS_LOG_COMPONENT_DEFINE ("SimbricksBase");

Adapter::Adapter()
    : sync(false), isListen(false), rescheduleSyncTx(false),
      pollInterval(500000), terminated(false), pool(nullptr)
{
}

Adapter::~Adapter()
{
    SimbricksBaseIfClose(&baseIf);
    if (isListen)
        SimbricksBaseIfUnlink(&baseIf);

    if (pool) {
        SimbricksBaseIfSHMPoolUnlink(pool);
        SimbricksBaseIfSHMPoolUnmap(pool);
    }
}

void Adapter::processInEvent()
{
    NS_LOG_FUNCTION (this);

    uint64_t now = curTick();

    /* run what we can */
    while (poll(now));

    /* if peer signaled terminated: no need to re-schedule */
    if (terminated)
        return;

    if (sync) {
        /* in sychronized mode we might need to wait till we get a message with
         * a timestamp allowing us to proceed */
        uint64_t nextTs;
        while ((nextTs = SimbricksBaseIfInTimestamp(&baseIf)) <= now) {
            poll(now);
        }

        if (terminated)
            return;

        inEvent = Simulator::Schedule (PicoSeconds (nextTs - now),
            &Adapter::processInEvent, this);
    } else {
        /* in non-synchronized mode just poll at fixed intervals */
        inEvent = Simulator::Schedule (PicoSeconds (pollInterval),
            &Adapter::processInEvent, this);
    }
}

void Adapter::processOutSyncEvent()
{
    NS_LOG_FUNCTION (this);
  
    uint64_t now = curTick();
    while (SimbricksBaseIfOutSync(&baseIf, now));
    uint64_t next_delay = SimbricksBaseIfOutNextSync(&baseIf) - now;
    outSyncEvent = Simulator::Schedule (PicoSeconds (next_delay),
            &Adapter::processOutSyncEvent, this);
}

void Adapter::rescheduleSync()
{
    if (terminated)
        return;

    Simulator::Cancel (outSyncEvent);
    uint64_t next_delay = SimbricksBaseIfOutNextSync(&baseIf) - curTick();
    outSyncEvent = Simulator::Schedule (PicoSeconds (next_delay),
            &Adapter::processOutSyncEvent, this);
}

void Adapter::startEvent()
{
    NS_LOG_FUNCTION (this);
    // wait for initialization of this adapter to be complete
    InitManager::get().waitReady(*this);
  
    // schedule first sync to be sent immediately
    if (sync)
        outSyncEvent = Simulator::ScheduleNow (
          &Adapter::processOutSyncEvent, this);
    // next schedule poll event
    inEvent = Simulator::ScheduleNow (&Adapter::processInEvent, this);
}

void Adapter::commonInit(const std::string &sock_path)
{
    NS_LOG_FUNCTION (this);
    SimbricksBaseIfDefaultParams(&params);
    initIfParams(params);

    params.sock_path = sock_path.c_str();
    params.blocking_conn = false;
    if (sync)
        params.sync_mode = kSimbricksBaseIfSyncRequired;
    else
        params.sync_mode = kSimbricksBaseIfSyncDisabled;

    if (SimbricksBaseIfInit(&baseIf, &params))
        NS_ABORT_MSG ("base init failed");

    Simulator::ScheduleNow (&Adapter::startEvent, this);
}

size_t Adapter::introOutPrepare(void *data, size_t maxlen)
{
    NS_LOG_FUNCTION (this);
    return 0;
}

void Adapter::introInReceived(const void *data, size_t len)
{
    NS_LOG_FUNCTION (this);
}

void Adapter::handleInMsg(volatile union SimbricksProtoBaseMsg *msg)
{
    NS_LOG_FUNCTION (this);
    // by default just ignore messages but mark them as done
    inDone(msg);
}

void Adapter::initIfParams(SimbricksBaseIfParams &p)
{
}

void Adapter::peerTerminated()
{
    NS_LOG_FUNCTION (this);

    terminated = true;
    if (sync)
      Simulator::Cancel (outSyncEvent);

    // poll event is currently being handled, and won't be re-scheduled
}


void Adapter::connect(const std::string &sock_path)
{
    NS_LOG_FUNCTION (this);

    commonInit(sock_path);

    if (SimbricksBaseIfConnect(&baseIf))
        NS_ABORT_MSG ("connecting failed");

    // register this adapter for the rest of the initialization
    // see init_manager.hh for rationale
    InitManager::get().registerAdapter(*this);
}

void Adapter::listen(const std::string &sock_path, const std::string &shm_path)
{
    NS_LOG_FUNCTION (this);
    commonInit(sock_path);

    pool = new SimbricksBaseIfSHMPool;
    if (SimbricksBaseIfSHMPoolCreate(pool, shm_path.c_str(),
            SimbricksBaseIfSHMSize(&params)))
        NS_ABORT_MSG ("creating SHM pool failed");

    if (SimbricksBaseIfListen(&baseIf, pool))
        NS_ABORT_MSG ("listening failed");

    isListen = true;

    // register this adapter for the rest of the initialization
    // see init_manager.hh for rationale
    InitManager::get().registerAdapter(*this);
}

void Adapter::Stop()
{
    NS_LOG_FUNCTION (this);

    if (sync)
        Simulator::Cancel (outSyncEvent);
    Simulator::Cancel (inEvent);
}

bool Adapter::poll(uint64_t now_ts)
{
    NS_LOG_FUNCTION (this);

    volatile union SimbricksProtoBaseMsg *msg =
        SimbricksBaseIfInPoll(&baseIf, now_ts);
    if (!msg)
        return false;

    // don't pass sync messages to handle msg function
    uint8_t ty = SimbricksBaseIfInType(&baseIf, msg);
    if (ty == SIMBRICKS_PROTO_MSG_TYPE_SYNC) {
        inDone(msg);
        return true;
    } else if (ty  == SIMBRICKS_PROTO_MSG_TYPE_TERMINATE) {
        peerTerminated();
        inDone(msg);
        return true;
    }

    handleInMsg(msg);
    return true;
}

} // namespace base
} // namespace simbricks
} // namespace gem5
