/*
 * FastMonitor.cc
 *
 *  Created on: Nov 27, 2012
 *      Author: aspataru
 */

#include "EventFilter/Utilities/interface/FastMonitor.h"
#include "EventFilter/Utilities/interface/JsonSerializable.h"
#include "EventFilter/Utilities/interface/FileIO.h"
#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <fstream>
#include <iostream>
#include <sstream>
#include <assert.h>
#include <sys/types.h>
#include <unistd.h>


using namespace jsoncollector;


FastMonitor::FastMonitor(std::string const& defPath, bool useSource, size_t nstreams, bool fastMonitoring) :
	defPath_(defPath),useSource_(useSource),nstreams_(nstreams)
{
  //get host and PID info
  if (useSource)
    getHostAndPID(sourceInfo_);
  currentGlobalLS_=0;
  for (size_t i=0;i<nstreams;i++) currentStreamLS_.push_back(0);
}


FastMonitor::~FastMonitor()
{
}


void FastMonitor::registerMonitorable(std::vector<JsonMonitorable*> *monitorables,
                                 std::string const& name,
                                 MonType type,
                                 OperationType op,
                                 EmptyMode em,
                                 SnapshotMode sm,
                                 bool isGlobal,
                                 size_t nbins,
                                 size_t expectedUpdates,
                                 size_t maxUpdates
                                 )

  size_t streams = isGlobal ? 1:nstreams_;
  monitorables = new std::vector<JsonMonitorable*>
  trackedVector_.emplace_back();
  TrackedMonitorable & t = trackedVector_.back();
  t.monitorables_ = monitorables;
  t.isGlobal_=isGlobal;
  t.nbins_=nbins;
  t.expectedUpdates_=expectedUpdates;
  t.maxUpdates_=maUpdates;


  for (size_t streamid=0;streamid<streams;streamid++)
  {
    switch (op) {
      case OP_SUM:
        assert(type!=TYPE_STRING);
        if (type==TYPE_INT) t.mon_.push_back(new IntJ(...));
        if (type==TYPE_DOUBLE) t.mon_.push_back(new DoubleJ(...));
        break;
      case OP_AVG:
        assert(type!=TYPE_STRING);
        if (type==TYPE_INT) t.mon_.push_back(new IntJ());
        if (type==TYPE_DOUBLE) t.mon_.push_back(new DoubleJ());
        break;
      case OP_SAME:
      case OP_SAMPLE:
        if (type==TYPE_STRING) t.mon_.push_back(new StringJ());
        if (type==TYPE_INT) t.mon_.push_back(new IntJ());
        if (type==TYPE_DOUBLE) t.mon_.push_back(new DoubleJ());
        break;
      case OP_HISTO:
        if (type==TYPE_INT)
          if (sm==SM_TIMER) t.mon_.push_back(new IntJ());
          else t.mon_.push_back(new HistoJ<IntJ>(nbins));
        if (type==TYPE_DOUBLE)
          if (sm==SM_TIMER) t.mon_.push_back(new DoubleJ());
          else t.mon_.push_back(new HistoJ<DoubleJ>(nbins));
        break;
      case OP_CAT:
        if (type==TYPE_STRING)
          if (sm==SM_TIMER) t.mon_.push_back(new StringJ());
          else t.mon_.push_back(new VectorJ<StringJ>(expectedUpdates,maxUpdates));
      case OP_APPEND:
        if (type==TYPE_INT)
          if (sm==SM_TIMER) t.mon_.push_back(new IntJ());
          else t.mon_.push_back(new VectorJ<IntJ>(expectedUpdates,maxUpdates));
        if (type==TYPE_DOUBLE)
          if (sm==SM_TIMER) t.mon_.push_back(new DoubleJ());
          else t.mon_.push_back(new VectorJ<DoubleJ>(expectedUpdates,maxUpdates));
        break;
      case OP_BINARYAND:
      case OP_BINARYOR:
        t.mon_.push_back(new IntJ());
        break;
      case OP_ADLER32:
        assert(isGlobal);//not supported per framework stream
        t.mon_.push_back(new IntJ());
        break; 
      default:
        assert(0);
    }
  }
  //todo:check fast mon and SM_TIMER with Histo, CAT and APPEND
  assert(monitorables->size());
  JsonMonitorable::typeCheck(type,op,*monitorables->at(0));

  MonitorableDefinition monDef(name,type,op,true,em,sm);
  if (!def_.get()) 
    def_.reset(new DataPointDefinition())
  def_.addMonitorableDefinition(monDef);

  //a copy of mon def
  t.monDef = monDef;

  //name map for quick access
  trackedMap_[name]=trackedVector.size()-1;


}

//fast path: no merge operation is performed //TODO
void FastMonitor::registerFastMonitorable(JsonMonitorable *monitorable,std::string const& name, OperationType overrideOperation)
{
  //alternative monitorable for fast mon
  trackFastMonitorable(monitorable,name);
}

std::string snapAndGetFastCSV() {
    if (!fastCollection_)
      fastCollection_ = new DataPointCollection( trackedVector_,def_,true,sourceInfo_);

}

void FastMonitor::snapTimer(unsigned int ls)
{

  recentSnaps_++;
  recentSnapsTimer_++;
  auto itr = collections_.find(ls);
  if (itr==collections_.end())
    itr = collections_.emplace(ls, trackedVector_,def_).first;
  itr->snapTimer();
  //fastMon:
  //csv = itr->getCurrentFastMonCSV()
}


//update for global variables as most of them are correct only at global EOL
void FastMonitor::snapGlobalEOL(unsigned int ls)
{
  recentSnaps_++;
  auto itr = collections_.find(ls);
  if (itr==collections_.end())
    itr = collections_.emplace(ls, trackedVector_,def_).first;
  itr->snapGlobalEOL();
}


//update atomic per-stream vars(e.g. event counters) not updating time-based measurements (mini/microstate)
void FastMonitor::snapStreamEOL(unsigned int ls, unsigned int streamID)
{
  recentSnaps_++;
  auto itr = collections_.find(ls);
  if (itr==collections_.end())
    itr = collections_.emplace(ls, trackedVector_,def_).first;
  itr->snapStreamEOL(streamID);
}

//TODO
std::string && FastMonitor::getCSVString(unsigned ls)
{

  auto itr = collections_.find(ls);
  if itr==collections_.end() return std::string();

  return itr->getFastCSV();
}


void FastMonitor::outputCSV(std::string const& path, std::string const& csvString)
{
  std::ofstream outputFile;
  outputFile.open(path.c_str(), std::fstream::out | std::fstream::trunc);
  outputFile << defPathFast_ << std::endl;
  outputFile << csvString << std::endl;
  outputFile.close();
}

//get one variable (caller must delete it later)
JsonMonitorable* FastMonitor::getMergedMonitorableForLS(std::string const& name, unsigned int ls)
{
  auto itr = collections_.find(ls);
  if itr==collections_.end() return nullptr;
  return  itr->mergeAndRetrieveValue(trackedMap_[name],ls);
}


bool FastMonitor::outputFullJSON(std::string const& path, unsigned int ls, bool log)
{
  if (log)
    LogDebug("FastMonitor") << "SNAP updates -: " <<  recentSnaps_ 
                            << " (by timer: " << recentSnapsTimer_
                            << ") in lumisection ";

  recentSnaps_ = recentSnapsTimer_ = 0;

  auto itr = collections_.find(ls);
  if itr==collections_.end() return false;

  Json::Value serializeRoot = itr_->mergeAndSerialize();

  Json::StyledWriter writer;
  std::string && result = writer.write(serializeRoot);
  FileIO::writeStringToFile(path, result);
  return true;
}


void FastMonitor::discardCollectedLS(unsigned int ls)
{
  if (collections_.find(ls)!=collections_.end())
    collections_.erase(ls);
    //todo:put in free queue
}


void FastMonitor::getHostAndPID(std::string& sHPid)
{
  std::stringstream hpid;
  int pid = (int) getpid();
  char hostname[128];
  gethostname(hostname, sizeof hostname);
  hpid << hostname << "_" << pid;
  sHPid = hpid.str();
}

