/*
 * DataPoint.cc
 *
 *  Created on: Sep 24, 2012
 *      Author: aspataru
 */

#include "EventFilter/Utilities/interface/DataPoint.h"

//#include <tbb/concurrent_vector.h>

#include <algorithm>
#include <assert.h>

//max collected updates per lumi
#define MAXUPDATES 200
#define MAXBINS

using namespace jsoncollector;

const std::string DataPoint::SOURCE = "source";
const std::string DataPoint::DEFINITION = "definition";
const std::string DataPoint::DATA = "data";

//todo:optimize with move operator
DataPoint::DataPoint(TrackedMonitorable tracked, bool trackingInstance, bool fastMon): tracked_(tracked),fastMon_(fastMon)
{

  assert(tracked_.mon_);
  size_t len = tracked_.mon_->size();
  if (!len) {assert(tracked_.mon_->size());}
  MonitorableDefinition & md = tracked.md_;
  //set tracking option
  tracked_.isTracking_ = trackingInstance;
  if (fastMon_) return;

  //clone types from tracked object 
  monitorable_ = tracked_.mon_->at(0)->cloneAggregationType(md.getOperation(), md.getSnapshotMode(), tracked_.nbins_, tracked.expectedUpdates_, tracked.maxUpdates_);
  if (len>1) {
    assert(tracked_.isGlobal_);
    for (size_t i=0;i<tracked_.mon_->size();i++) {
      streamMonitorables_.push_back(tracked_.mon_->at(i)->cloneAggregationType(md.getOperation(), md.getSnapshotMode(), tracked_.nbins_, tracked.expectedUpdates_, tracked.maxUpdates_));
    }
    useGlobalCopy_=false;
  }
}


//make non-tracking instance
DataPoint::DataPoint(TrackedMonitorable tracked, JsonMonitorable* aggregated): tracked_(tracked),fastMon_(false)
{
  tracked_.isTracking_=false;
  if (!aggregated) return;
  //assert(aggregated);
  monitorable_ = aggregated->cloneType();
}


//make non-tracking instance
DataPoint::DataPoint(TrackedMonitorable tracked, Json::Value const& data): tracked_(tracked),fastMon_(false)
{
  tracked_.isTracking_=false;
  monitorable_ = JsonMonitorable::createMonitorable(data,tracked_.md_.getType(),tracked_.md_.getOperation());
}



void DataPoint::destroy()
{
  if (monitorable_) delete monitorable_;
  for (auto m:streamMonitorables_) delete m;
  //TODO:fast
}

void DataPoint::reset()
{
  if (monitorable_) monitorable_->resetValue();
  for (auto m:streamMonitorables_) if (m) m->resetValue();
}

//get current tracked variable for fast-mon
std::string DataPoint::snapAndGetFast()
{
  assert(tracked_.mon_ && tracked_.mon_->size());
  return tracked_.mon_->at(0)->toString();
}

void DataPoint::snapTimer()
{
  if (tracked_.md_.getSnapshotMode()!=SM_TIMER) return;
  isCached_=false;
  if (useGlobalCopy_) monitorable_->update(tracked_.mon_->at(0),tracked_.md_.getSnapshotMode(),tracked_.md_.getOperation());
  else for (size_t i=0;i<streamMonitorables_.size();i++) streamMonitorables_[i]->update(tracked_.mon_->at(i),tracked_.md_.getSnapshotMode(),tracked_.md_.getOperation());
}

void DataPoint::snapGlobalEOL()
{
  if (tracked_.md_.getSnapshotMode()!=SM_EOL || !tracked_.isGlobal_) return;
  isCached_=false;
   monitorable_->update(tracked_.mon_->at(0),tracked_.md_.getSnapshotMode(),tracked_.md_.getOperation());
}

void DataPoint::snapStreamEOL(unsigned int streamID)
{
  if (tracked_.md_.getSnapshotMode()!=SM_EOL || tracked_.isGlobal_) return;
  isCached_=false;
//  if (useGlobalCopy_) monitorable_->update(tracked_.mon_->at(0),tracked_.md_.getSnapshotMode(),tracked_.md_.getOperation());//invalid!

  //else for (size_t i=0;i<streamMonitorables_.size();i++) streamMonitorables_[i]->update(tracked_.mon_->at(i),tracked_.md_.getSnapshotMode(),tracked_.md_.getOperation());
  streamMonitorables_[streamID]->update(tracked_.mon_->at(streamID),tracked_.md_.getSnapshotMode(),tracked_.md_.getOperation());

}

//for FMS
Json::Value & DataPoint::getJsonValue()
{
  if (!isCached_) {
    isCached_=true;
    if (!useGlobalCopy_) {
      monitorable_->resetUpdates();
      if (!JsonMonitorable::mergeData(monitorable_,&streamMonitorables_,tracked_.md_.getType(),tracked_.md_.getOperation())) {
        value_ = "N/A";
        return value_;
      }
    }
  }
  if (!monitorable_->getUpdates() && tracked_.md_.getEmptyMode() == EM_NA)
      value_ = "N/A";
  else
    value_= monitorable_->toJsonValue();
  return value_;
}

//For FMS (processed)
JsonMonitorable* DataPoint::mergeAndRetrieveMonitorable()
{
  if (!useGlobalCopy_ && !isCached_)
    JsonMonitorable::mergeData(monitorable_,&streamMonitorables_,tracked_.md_.getType(),tracked_.md_.getOperation());
  return monitorable_->clone();
}

//for standalone binary
bool DataPoint::mergeAndSerializeMonitorables(Json::Value &root,std::vector<JsonMonitorable*>* vec)
{
  isCached_=true;
  monitorable_->resetUpdates();
  if (vec->size() && mergeMonitorables(vec)) { //todo:handle failures
    if ( tracked_.md_.getEmptyMode()==EM_NA && !monitorable_->getUpdates())
      value_ = "N/A";
    else
      value_= monitorable_->toJsonValue();
    root[DATA].append(value_);
    return true;
  }
  else {
    value_ = "N/A";
    root[DATA].append(value_);
    return false;
  }
}

//for stream modules
bool DataPoint::mergeMonitorables(std::vector<JsonMonitorable*> *vec)
{
  assert(useGlobalCopy_ && !tracked_.mon_);
  return JsonMonitorable::mergeData(monitorable_, vec, tracked_.md_.getType(), tracked_.md_.getOperation());
  //return JsonMonitorable::mergeData(monitorable_, vec, tracked_.md_.getType(), tracked_.md_.getOperation());
  isCached_=true;
}

