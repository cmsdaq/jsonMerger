/*
 * FastMonitor.h
 *
 *  Created on: Nov 27, 2012
 *      Author: aspataru
 */

#ifndef FASTMONITOR_H_
#define FASTMONITOR_H_

#include "EventFilter/Utilities/interface/DataPointCollection.h"

#include <unordered_set>

namespace jsoncollector {

class FastMonitor {

public:

  FastMonitor::FastMonitor(std::string const& defPath, bool useSource, size_t nstreams);

  ~FastMonitor();

void registerMonitorable(std::vector<JsonMonitorable*> &monitorables,
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

  //register fastPath global monitorable under same name
  //void registerFastMonitorable(JsonMonitorable *newMonitorable,name);

  // fetches timer based / fast update
  void snapTimer(unsigned int ls);
  void snap(unsigned int ls);

  //only update global variables (invoked at global EOL)
  void snapGlobalEoL(unsigned int ls);

  //only updates atomic vectors (for certain stream - at stream EOL)
  void snapStreamEoL(unsigned int ls, unsigned int streamID);

  //let monitor know which global lumi was last opened
  void updateGlobalLS(unsigned int ls) {currentGlobalLS_=ls;}

  //let monitor know which streams is in which ls
  void updateStreamLS(unsigned int ls, unsigned int streamID) {currentStreamLS_[streamID]=ls;}

  //fastpath CSV string
  std::string getCSVString();//TODO

  //fastpath file output
  void outputCSV(std::string const& path, std::string const& csvString);//TODO

  //provide merged variable back to user
  JsonMonitorable* getMergedMonitorableForLS(std::string const& name, unsigned int ls);

  // merges and outputs everything collected for the given stream to JSON file
  bool outputFullJSON(std::string const& path, unsigned int lumi, bool log=true);

  //discard what was collected for a lumisection
  void discardCollectedLS(unsigned int ls);

  //this is added to the JSON file
  void getHostAndPID(std::string& sHPid);

private:

  std::string defPath_;
  bool useSource_;
  //bool haveFastPath_=false;
  unsigned int nstreams_;
  std::string sourceInfo_;

  unsigned int currentGlobalLS_=0;
  std::vector<unsigned int> currentStreamLS_;

  std::vector<TrackedMonitorable> trackedVector_;
  std::map<unsigned int, DataPointCollection> collections_;

  //todo:
  //tbb::concurrent_queue<DataPointCollection> freeCollections_;

  std::shared_ptr<DataPointDefinition> def_;
  std::map<std::string,std::shared_ptr<DataPointDefinition>> defMap_;

  unsigned int recentSnaps_ = 0;
  unsigned int recentSnapsTimer_ = 0;

  //TODO:detect doubles
  //std::unordered_set<std::string> uids_;

};

}

#endif /* FASTMONITOR_H_ */
