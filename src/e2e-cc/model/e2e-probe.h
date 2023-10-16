/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */

/*
 * Copyright 2023 Max Planck Institute for Software Systems
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

#ifndef E2E_PROBE_H
#define E2E_PROBE_H

#include "e2e-component.h"

namespace ns3
{

/* ... */

class E2EProbe : public E2EComponent
{
  public:
    E2EProbe(const E2EConfig& config);

    virtual void Install(Ptr<Application> application) = 0;
};

template<typename T>
class E2EPeriodicSampleProbe : public E2EProbe
{
  public:
    E2EPeriodicSampleProbe(const E2EConfig& config);

    void Install(Ptr<Application> application) override {}

    static void UpdateValue(T value, Ptr<E2EPeriodicSampleProbe<T>> probe)
    {
        probe->m_value = value;
    }

    static void AddValue(T value, Ptr<E2EPeriodicSampleProbe<T>> probe)
    {
        probe->m_value += value;
    }

    void WriteData();

    T m_value;

  private:
    std::ostream* m_output;
    std::string_view m_unit;
    Time m_interval;
};

template <typename T>
inline E2EPeriodicSampleProbe<T>::E2EPeriodicSampleProbe(const E2EConfig& config) : E2EProbe(config)
{
    Time startTime;
    m_output = &std::cout;
    if (auto header {config.Find("Header")}; header)
    {
        *m_output << *header;
    }
    if (auto unit {config.Find("Unit")}; unit)
    {
        m_unit = *unit;
    }
    if (auto interval {config.Find("Interval")}; interval)
    {
        m_interval = Time(std::string(*interval));
    }
    else
    {
        m_interval = MilliSeconds(500);
    }
    if (auto start {config.Find("Start")}; start)
    {
        startTime = Time(std::string(*start));
    }
    else
    {
        startTime = m_interval;
    }

    Simulator::Schedule(startTime, &E2EPeriodicSampleProbe<T>::WriteData, this);
}

template <typename T>
inline void
E2EPeriodicSampleProbe<T>::WriteData()
{
    *m_output << m_value << m_unit << std::endl;
    Simulator::Schedule(m_interval, &E2EPeriodicSampleProbe<T>::WriteData, this);
}

template<typename T>
void TraceOldNewValue(void func(T value, Ptr<E2EPeriodicSampleProbe<T>> probe),
                      Ptr<E2EPeriodicSampleProbe<T>> probe,
                      T oldValue,
                      T newValue)
{
    func(newValue, probe);
}

void TraceRx(void func(uint32_t, Ptr<E2EPeriodicSampleProbe<uint32_t>>),
             Ptr<E2EPeriodicSampleProbe<uint32_t>> probe,
             Ptr<const Packet> packet,
             const Address& address);

template<typename T, typename U>
void ConnectTraceToSocket(Ptr<T> application,
                          const std::string& trace,
                          Ptr<E2EPeriodicSampleProbe<U>> probe,
                          void func(U, Ptr<E2EPeriodicSampleProbe<U>>))
{
    Ptr<Socket> socket = application->GetSocket();
    NS_ABORT_MSG_UNLESS(socket, "No socket found for application");
    socket->TraceConnectWithoutContext(trace, MakeBoundCallback(TraceOldNewValue<U>, func, probe));
}

} // namespace ns3

#endif /* E2E_PROBE_H */