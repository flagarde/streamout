/** \file BufferLooper.h
*  \copyright 2022 G.Grenier F.Lagarde
*/

#pragma once

#include "Buffer.h"
#include "BufferLooperCounter.h"
#include "DetectorId.h"
#include "Formatters.h"
#include "RawBufferNavigator.h"
#include "Timer.h"
#include "Words.h"

#include <algorithm>
#include <cassert>
#include <memory>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/spdlog.h>
#include <vector>
// function to loop on buffers

template<typename SOURCE, typename DESTINATION> class BufferLooper
{
public:
  BufferLooper(SOURCE& source, DESTINATION& dest, bool debug = false) : m_Source(source), m_Destination(dest), m_Debug(debug)
  {
    m_Logger = spdlog::create<spdlog::sinks::null_sink_mt>("streamout");
    if(!spdlog::get("streamout")) { spdlog::register_logger(m_Logger); }
    m_Source.setLogger(m_Logger);
    m_Destination.setLogger(m_Logger);
  }

  void addSink(const spdlog::sink_ptr& sink, const spdlog::level::level_enum& level = spdlog::get_level())
  {
    sink->set_level(level);
    m_Sinks.push_back(sink);
    m_Logger = std::make_shared<spdlog::logger>("streamout", begin(m_Sinks), end(m_Sinks));
    m_Source.setLogger(m_Logger);
    m_Destination.setLogger(m_Logger);
  }

  void loop(const std::uint32_t& m_NbrEventsToProcess = 0)
  {
    Timer timer;
    timer.start();
    m_Source.start();
    m_Destination.start();
    RawBufferNavigator bufferNavigator;
    while(m_Source.nextEvent() && m_NbrEventsToProcess >= m_NbrEvents)
    {
      /// START EVENT ///
      m_Source.startEvent();
      m_Destination.startEvent();
      ///////////////////

      m_Logger->warn("===*** Event number {} ***===", m_NbrEvents);
      while(m_Source.nextDIFbuffer())
      {
        const Buffer& buffer = m_Source.getSDHCALBuffer();
        bufferNavigator.setBuffer(buffer);

        bit8_t* debug_variable_1 = buffer.end();
        bit8_t* debug_variable_2 = bufferNavigator.getDIFBuffer().end();
        if(debug_variable_1 != debug_variable_2) m_Logger->info("DIF BUFFER END {} {}", fmt::ptr(debug_variable_1), fmt::ptr(debug_variable_2));
        if(m_Debug) assert(debug_variable_1 == debug_variable_2);

        if(std::find(m_DetectorIDs.begin(), m_DetectorIDs.end(), static_cast<DetectorID>(bufferNavigator.getDetectorID())) == m_DetectorIDs.end())
        {
          m_Logger->trace("{}", bufferNavigator.getDetectorID());
          continue;
        }

        /// START DIF ///
        m_Source.startDIF();
        m_Destination.startDIF();
        ///////////////////

        std::int32_t idstart = bufferNavigator.getStartOfDIF();
        if(m_Debug && idstart == -1) m_Logger->info(to_hex(buffer));
        c.DIFStarter[idstart]++;
        if(!bufferNavigator.validBuffer())
        {
          m_Logger->error("!bufferNavigator.validBuffer()");
          continue;
        }
        DIFPtr& d = bufferNavigator.getDIFPtr();
        c.DIFPtrValueAtReturnedPos[bufferNavigator.getDIFBufferStart()[d.getGetFramePtrReturn()]]++;
        if(m_Debug) assert(bufferNavigator.getDIFBufferStart()[d.getGetFramePtrReturn()] == 0xa0);
        c.SizeAfterDIFPtr[bufferNavigator.getSizeAfterDIFPtr()]++;
        m_Destination.processDIF(d);
        for(std::size_t i = 0; i < d.getNumberOfFrames(); ++i)
        {
          /// START FRAME ///
          m_Source.startFrame();
          m_Destination.startFrame();
          ///////////////////
          m_Destination.processFrame(d, i);
          for(std::size_t j = 0; j < DU::NUMBER_PAD; ++j)
          {
            m_Source.startPad();
            m_Destination.startPad();
            m_Destination.processPadInFrame(d, i, j);
            m_Source.endPad();
            m_Destination.endPad();
          }
          /// START FRAME ///
          m_Source.endFrame();
          m_Destination.endFrame();
          ///////////////////
        }

        bool processSC = false;
        if(bufferNavigator.hasSlowControlData())
        {
          c.hasSlowControl++;
          processSC = true;
        }
        if(bufferNavigator.badSCData())
        {
          c.hasBadSlowControl++;
          processSC = false;
        }
        if(processSC) { m_Destination.processSlowControl(bufferNavigator.getSCBuffer()); }

        Buffer eod = bufferNavigator.getEndOfAllData();
        c.SizeAfterAllData[eod.size()]++;
        bit8_t* debug_variable_3 = eod.end();
        if(debug_variable_1 != debug_variable_3) m_Logger->info("END DATA BUFFER END {} {}", fmt::ptr(debug_variable_1), fmt::ptr(debug_variable_3));
        if(m_Debug) assert(debug_variable_1 == debug_variable_3);
        if(eod.size() != 0) m_Logger->info("End of Data remaining stuff : {}", to_hex(eod));

        int nonzeroCount = 0;
        for(bit8_t* it = eod.begin(); it != eod.end(); it++)
          if(static_cast<int>(*it) != 0) nonzeroCount++;
        c.NonZeroValusAtEndOfData[nonzeroCount]++;
        /// START DIF ///
        m_Source.endDIF();
        m_Destination.endDIF();
        ///////////////////
      }  // end of DIF while loop
      m_Logger->warn("***=== Event number {} ===***", m_NbrEvents);
      m_NbrEvents++;
      /// START EVENT ///
      m_Source.endEvent();
      m_Destination.endEvent();
      ///////////////////
    }  // end of event while loop
    m_Destination.end();
    m_Source.end();
    timer.stop();
    fmt::print("=== elapsed time {}ms ({}ms/event) ===\n", timer.getElapsedTime() / 1000, timer.getElapsedTime() / (1000 * m_NbrEvents));
  }
  void                            printAllCounters() { c.printAllCounters(); }
  std::shared_ptr<spdlog::logger> log() { return m_Logger; }

  void setDetectorIDs(const std::vector<DetectorID>& detectorIDs) { m_DetectorIDs = detectorIDs; }

private:
  std::vector<DetectorID>         m_DetectorIDs;
  std::shared_ptr<spdlog::logger> m_Logger{nullptr};
  std::vector<spdlog::sink_ptr>   m_Sinks;
  BufferLooperCounter             c;
  SOURCE&                         m_Source{nullptr};
  DESTINATION&                    m_Destination{nullptr};
  bool                            m_Debug{false};
  std::uint32_t                   m_NbrEvents{1};
};
