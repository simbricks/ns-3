/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Copyright 2024 Max Planck Institute for Software Systems
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

#ifndef SIMBRICKS_BASE_H
#define SIMBRICKS_BASE_H

#include <set>
#include <stdint.h>
#include <simbricks/base/cxxatomicfix.h>

#include "ns3/event-id.h"
#include "ns3/nstime.h"
#include "ns3/simulator.h"

namespace ns3 {
namespace simbricks {

extern "C" {
#include <simbricks/base/if.h>
};

namespace base {

class InitManager;
class Adapter
{
  private:
    friend class InitManager;

    bool sync;
    bool isListen;
    bool rescheduleSyncTx;
    uint64_t pollInterval;
    bool terminated;
    EventId inEvent;
    EventId outSyncEvent;
    struct SimbricksBaseIf baseIf;
    struct SimbricksBaseIfSHMPool *pool;
    struct SimbricksBaseIfParams params;

    void processInEvent();
    void processOutSyncEvent();
    void rescheduleSync();
    void startEvent();

    void commonInit(const std::string &sock_path);
  protected:
    uint64_t curTick() {
      return Simulator::Now().ToInteger(Time::PS);
    }

    virtual size_t introOutPrepare(void *data, size_t maxlen);
    virtual void introInReceived(const void *data, size_t len);
    virtual void handleInMsg(volatile union SimbricksProtoBaseMsg *msg);
    virtual void initIfParams(SimbricksBaseIfParams &p);
    virtual void peerTerminated();

  public:
    Adapter();
    virtual ~Adapter();

    void cfgSetSync(bool x) {
        sync = x;
    }

    void cfgSetPollInterval(uint64_t i) {
        pollInterval = i;
    }

    void cfgSetRescheduleSyncTx(bool x) {
        rescheduleSyncTx = x;
    }

    void connect(const std::string &sock_path);
    void listen(const std::string &sock_path, const std::string &shm_path);
    void Stop();

    bool poll(uint64_t now_ts);

    void inDone(volatile union SimbricksProtoBaseMsg *msg) {
        SimbricksBaseIfInDone(&baseIf, msg);
    }

    uint8_t inType(volatile union SimbricksProtoBaseMsg *msg) {
        return SimbricksBaseIfInType(&baseIf, msg);
    }

    volatile union SimbricksProtoBaseMsg *outAlloc() {
        volatile union SimbricksProtoBaseMsg *msg;
        if (terminated)
            return nullptr;

        do {
            msg = SimbricksBaseIfOutAlloc(&baseIf,
              Simulator::Now ().ToInteger (Time::PS));
        } while (!msg);
        return msg;
    }

    void outSend(volatile union SimbricksProtoBaseMsg *msg, uint8_t ty) {
        SimbricksBaseIfOutSend(&baseIf, msg, ty);
        if (sync && rescheduleSyncTx) {
            rescheduleSync();
        }
    }
};

template <typename TMI, typename TMO>
class GenericBaseAdapter : public Adapter
{
  public:
    class Interface
    {
      public:
        virtual size_t introOutPrepare(void *data, size_t maxlen) = 0;
        virtual void introInReceived(const void *data, size_t len) = 0;
        virtual void handleInMsg(volatile TMI *msg) = 0;
        virtual void initIfParams(SimbricksBaseIfParams &p) = 0;
        virtual void peerTerminated() = 0;
    };

  protected:
    Interface &intf;

    virtual size_t introOutPrepare(void *data, size_t len) {
        return intf.introOutPrepare(data, len);
    }

    virtual void introInReceived(const void *data, size_t len) {
        intf.introInReceived(data, len);
    }

    virtual void handleInMsg(volatile union SimbricksProtoBaseMsg *msg) {
        intf.handleInMsg((volatile TMI *) msg);
    }

    virtual void initIfParams(SimbricksBaseIfParams &p) {
        Adapter::initIfParams(p);
        intf.initIfParams(p);
    }

    virtual void peerTerminated() {
      Adapter::peerTerminated();
      intf.peerTerminated();
    }

  public:
    GenericBaseAdapter(Interface &intf_)
      : Adapter(), intf(intf_) {}

    virtual ~GenericBaseAdapter() = default;

    void inDone(volatile TMI *msg) {
        Adapter::inDone((volatile union SimbricksProtoBaseMsg *) msg);
    }

    uint8_t inType(volatile TMI *msg) {
        return Adapter::inType((volatile union SimbricksProtoBaseMsg *) msg);
    }

    volatile TMO *outAlloc() {
        return (volatile TMO *) Adapter::outAlloc();
    }

    void outSend(volatile TMO *msg, uint8_t ty) {
        Adapter::outSend((volatile union SimbricksProtoBaseMsg *) msg, ty);
    }
};


} /* namespace base */
} /* namespace simbricks */
} /* namespace ns3 */

#endif /* SIMBRICKS_BASE_H */

