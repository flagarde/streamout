/** \file SDHCAL_buffer_LoopCounter.cc
 *  \copyright 2022 G.Grenier F.Lagarde
 */

#include "SDHCAL_buffer_LoopCounter.h"

#include <fmt/core.h>

void SDHCAL_buffer_LoopCounter::printAllCounters()
{
  fmt::print("BUFFER LOOP FINAL STATISTICS : \n");
  printCounter("Start of DIF header", DIFStarter);
  printCounter("Value after DIF data are processed", DIFPtrValueAtReturnedPos);
  printCounter("Size remaining in buffer after end of DIF data", SizeAfterDIFPtr);
  fmt::print("Number of Slow Control found {}  out of which {} are bad\n", hasSlowControl, hasBadSlowControl);
  printCounter("Size remaining after all of data have been processed", SizeAfterAllData);
  printCounter("Number on non zero values in end of data buffer", NonZeroValusAtEndOfData);
}

void SDHCAL_buffer_LoopCounter::printCounter(const std::string& description, const std::map<int, int>& m)
{
  std::string out{"statistics for " + description + " : \n"};
  for(std::map<int, int>::const_iterator it = m.begin(); it != m.end(); it++)
  {
    if(it != m.begin()) out += ",";
    out += " [" + std::to_string(it->first) + "]=" + std::to_string(it->second);
  }
  out += "\n";
  fmt::print(out);
}
