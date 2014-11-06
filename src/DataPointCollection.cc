/*
 * DataPointCollection.cc
 *
 *  Created on: Oct 21, 2014
 *      Author: aspataru,smorovic
 */

#include "EventFilter/Utilities/interface/DataPointCollection.h"
#include "EventFilter/Utilities/interface/JSONSerializer.h"
#include "EventFilter/Utilities/interface/FileIO.h"

#include <algorithm>
#include <fstream>
#include <assert.h>


//max collected updates per lumi
#define MAXUPDATES 200
#define MAXBINS

using namespace jsoncollector;

const std::string DataPointCollection::SOURCE = "source";
const std::string DataPointCollection::DEFINITION = "definition";
const std::string DataPointCollection::DATA = "data";

//constructor for taking snapshots of tracked variables
DataPointCollection::DataPointCollection(std::vector<TrackedMonitorable> * trackedVector,bool fastMon,std::string *sourcePtr)
:sourcePtr_(sourcePtr),trackedVector_(trackedVector),fastMon_(fastMon)
{
  data_.reserve(trackedVector->size());
  for ( auto &t : *trackedVector)
  {
    //object copy
    data_.emplace_back(t,true,fastMon);
  }
  isInitialized_=true;
}

//constructor for parsing json files containing definition field
DataPointCollection::DataPointCollection(std::string & filename, std::map<std::string,DataPointDefinition*> *defMap,std::string *sourcePtr)
:filename_(filename),sourcePtr_(sourcePtr),defMap_(defMap)
{
  Json::Value root;
  std::string s;
  bool readSuccess = FileIO::readStringFromFile(filename, s);
  if (!readSuccess) {
    std::cout << "Could not read file " << filename;
    return;
  }

  isInitialized_ = JSONSerializer::deserialize(this,s);
}

//reset all snapped data
void DataPointCollection::reset() {
  for (auto & dp : data_) dp.reset();
}

DataPointCollection::~DataPointCollection() {
  for (auto &d :data_) d.destroy();
}

bool DataPointCollection::serialize(Json::Value& root)
{
  if (sourcePtr_ && sourcePtr_->size())  root[SOURCE] = *sourcePtr_;
  else if (source_.size()) root[SOURCE] = source_;

  if (definition_.size())
    root[DEFINITION] = definition_;
  else if (def_) 
    root[DEFINITION]= definition_ = def_->getDefFilePath();

  for (unsigned int i=0;i<data_.size();i++) {
//    Json::Value &ref = data_[i].getJsonValue();
    root[DATA].append(data_[i].getJsonValue());
  }
  return true;
}

bool DataPointCollection::deserialize(Json::Value& root)
{
  source_ = root.get(SOURCE, "").asString();
  definition_ = root.get(DEFINITION, "").asString();
  MonType type = TYPE_UNKNOWN;
  OperationType op = OP_UNKNOWN;
  if (defMap_ && !def_) {
    auto itr = defMap_->find(DataPointDefinition::definitionPathWithoutPid(definition_));
    if (itr == defMap_->end()) {
      auto dpd = new DataPointDefinition();
      std::string directory;
      const size_t last_slash_idx = filename_.rfind('/');
      if (std::string::npos != last_slash_idx)
        directory = filename_.substr(0, last_slash_idx);
      if (DataPointDefinition::getDataPointDefinitionFor(definition_,dpd,nullptr,defMap_,&directory))
        def_=dpd;
      else delete dpd;
    }
    else def_=itr->second;
  }

  if (root.get(DATA, "").isArray()) {
    unsigned int size = root.get(DATA, "").size();
    data_.reserve(size);
    for (unsigned int i = 0; i < size; i++)
    {
      if (def_) {
        MonitorableDefinition *mdp = def_->getMonitorableDefinition(i);
        if (!mdp) {
          std::cout << "No definition for field " << i << std::endl;
          return false;//TODO exception
        }
        TrackedMonitorable t(*mdp);
        data_.emplace_back(t,root.get(DATA, "")[i]);
      }
      else {
        //still allow to deserialize without valid definition
        MonitorableDefinition md("",type,op,false);
        TrackedMonitorable t(md);
        if (!JsonMonitorable::typeAndOperationCheck(type,op)) {
          std::cout << "Definition file not found or not valid: " << definition_ << std::endl;
          return false;//TODO:throw exception
        }
        data_.emplace_back(t,root.get(DATA, "")[i]);
      }
    }
  }
  return true;
}

void DataPointCollection::snapTimer()
{
  for (auto & d:data_) d.snapTimer();
}

void DataPointCollection::snapGlobalEOL()
{
  for (auto & d:data_) d.snapGlobalEOL();
}

void DataPointCollection::snapStreamEOL(unsigned int streamID)
{
 for (auto & d:data_) d.snapStreamEOL(streamID);
}

JsonMonitorable* DataPointCollection::mergeAndRetrieveMonitorable(std::string const& name)
{
  size_t i;
  if (!def_ || !def_->getIndex(name,i)) return nullptr;
  return data_[i].mergeAndRetrieveMonitorable();
}

JsonMonitorable* DataPointCollection::mergeAndRetrieveMonitorable(size_t index)
{
  if (index>=data_.size()) return nullptr;
  //assume the caller takes care of deleting the object
  return data_[index].mergeAndRetrieveMonitorable();
}

Json::Value DataPointCollection::mergeAndSerialize()
{
  Json::Value root;

  //todo:exception etc.
  assert(serialize(root));
  return root;
}

bool DataPointCollection::mergeCollections(std::vector<DataPointCollection*> const& collections)
{
  //assert(nStreams_==1);//function is allowed only for this case
  assert(collections.size());
  if (!data_.size()) {
    def_ = collections[0]->getDPD();
    for (size_t i=0;i<collections[0]->getData().size();i++) {
      data_.emplace_back(collections[0]->getData().at(i).getTracked(),collections[0]->getData().at(i).getMonitorable());
    }
  }
  std::vector<JsonMonitorable*> dpArray(collections.size());
  for (size_t i=0;i<data_.size();i++) {
    //build array..
    for (size_t j=0;j<collections.size();j++) {
      auto dataPtr = collections[j]->getDataAt(i);
      if (!dataPtr) return false;
      dpArray[j] = dataPtr->getMonitorable();
      if (dpArray[j]==nullptr) return false;
    }
//    data_[i].mergeAndSerializeMonitorables(&dpArray);
    if (!data_[i].mergeMonitorables(&dpArray)) return false;
  }
  return true;
}

bool DataPointCollection::mergeTrackedCollections(std::vector<DataPointCollection*> const& collections)
{
  //assert(nStreams_==1);//function is allowed only for this case
  assert(collections.size());
  if (!data_.size()) {
    def_ = collections[0]->getDPD();
    for (size_t i=0;i<collections[0]->getData().size();i++) {
      if (!collections[0]->getData().at(i).getMonitorable()) return false;//TODO...
      data_.emplace_back(collections[0]->getData().at(i).getTracked(),collections[0]->getData().at(i).getMonitorable());
    }
  }
  std::vector<JsonMonitorable*> dpArray(collections.size());
  for (size_t i=0;i<data_.size();i++) {
    //build array..
    for (size_t j=0;j<collections.size();j++) {
      auto dataPtr = collections[j]->getDataAt(i);
      if (!dataPtr) return false;
      auto mvec = dataPtr->getTracked().mon_;
      if (mvec || !mvec->size()) return false;
      dpArray[j] = mvec->at(0);
      if (dpArray[j]==nullptr) return false;
    }
    data_[i].mergeMonitorables(&dpArray);
  }
  return true;
}
