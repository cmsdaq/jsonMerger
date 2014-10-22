/*
 * DataPoint.h
 *
 *  Created on: Sep 24, 2012
 *      Author: aspataru
 */

#ifndef DATAPOINT_H_
#define DATAPOINT_H_

#include "EventFilter/Utilities/interface/JsonMonitorable.h"
#include "EventFilter/Utilities/interface/DataPointDefinition.h"

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <stdint.h>
#include <assert.h>

namespace jsoncollector {

struct TrackedMonitorable {
  TrackedMonitorable(MonitorableDefinition md):md_(md) {}
  MonitorableDefinition md_;
  //vector of stream copies (one if global)
  std::vector<JsonMonitorable*> *mon_ = nullptr;
  bool isGlobal_ = true;
  size_t nbins_=0;
  size_t expectedUpdates_=0;
  size_t maxUpdates_=0;
  bool isTracking_=false;
  /* 
  TrackedMonitorable(TrackedMonitorable m):
    md_(m.md_),
    mon_(m.mon_),
    isGlobal(m.isGlobal_),
    nbins_(m.nbins_),
    expectedUpdates_(m.expectedUpdates_),
    maxUpdates_(m.maxUpdates_) {}
  */
};
 
class DataPoint {

public:

  //DataPoint() { }
  DataPoint(TrackedMonitorable tracked, bool trackingInstance=true, bool fastMon=false);
  DataPoint(TrackedMonitorable tracked, JsonMonitorable* aggregated);
  DataPoint(TrackedMonitorable tracked, Json::Value const& data);

  void destroy();

  //take new update for lumi
  void snapTimer();
  void snapGlobalEOL();
  void snapStreamEOL(unsigned int streamID);

  void snap() {snapTimer();}

  //serialize to JSON value
  Json::Value & getJsonValue();

  //pointed object should be available until discard
  JsonMonitorable* mergeAndRetrieveMonitorable();

  //merge monitorable vector into this data point
  bool mergeAndSerializeMonitorables(Json::Value &root,std::vector<JsonMonitorable*>* vec);

  //
  bool mergeMonitorables(std::vector<JsonMonitorable*> *vec);

  TrackedMonitorable const& getTracked() const {return tracked_;}

  JsonMonitorable* getMonitorable() const {return monitorable_;}

  bool usesGlobalCopy() {return useGlobalCopy_;}

  void reset();


  std::string snapAndGetFast();

  static const std::string SOURCE;
  static const std::string DEFINITION;
  static const std::string DATA;

protected:
  //std::string name_;
  JsonMonitorable *monitorable_=nullptr;
  std::vector<JsonMonitorable*> streamMonitorables_;

  TrackedMonitorable tracked_;
  bool useGlobalCopy_=true;
  bool fastMon_=false;

  //json value cache
  Json::Value value_;
  bool isCached_=false;

};

}//jsoncollector

#endif /* DATAPOINT_H_ */
