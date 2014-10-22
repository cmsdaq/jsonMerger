/*
 * DataPoint.h
 *
 *  Created on: Sep 24, 2012
 *      Author: aspataru
 */

#ifndef DATAPOINTCOLLECTION_H_
#define DATAPOINTCOLLECTION_H_

#include "EventFilter/Utilities/interface/DataPoint.h"
#include "EventFilter/Utilities/interface/JsonSerializable.h"

#include <string>
#include <vector>
#include <memory>
#include <atomic>
#include <stdint.h>
#include <assert.h>

namespace jsoncollector {


class DataPointCollection: public JsonSerializable {

public:

	DataPointCollection(std::string * sourcePtr=nullptr):sourcePtr_(sourcePtr) { }

	DataPointCollection(std::vector<TrackedMonitorable> *,bool fastMon=false, std::string *sourcePtr=nullptr);

	//DataPointCollection(DataPointDefinition * def):def_(def) {}

        DataPointCollection(std::string & filename, std::map<std::string,DataPointDefinition*> *defMap=nullptr,std::string *sourcePtr=nullptr);

	~DataPointCollection();

	/**
	 * JSON serialization procedure for this class
	 */

	virtual bool serialize(Json::Value& root);

	/**
	 * JSON deserialization procedure for this class
	 */
	virtual bool deserialize(Json::Value& root);

	std::string const& getDefinition() const {return definition_;}
        DataPointDefinition* getDPD() const {return def_;}
	std::vector<DataPoint> const& getData() const {return data_;}

	//take new update for lumi
	void snapTimer();
	void snapGlobalEOL();
	void snapStreamEOL(unsigned int streamID);
	void snap() {snapTimer();}

	//fastpath (not implemented now)
	//std::string fastOutCSV();

        JsonMonitorable* mergeAndRetrieveMonitorable(std::string const& name);
        JsonMonitorable* mergeAndRetrieveMonitorable(size_t index);
        Json::Value mergeAndSerialize();
        bool mergeCollections(std::vector<DataPointCollection*> const& collections);
        bool mergeTrackedCollections(std::vector<DataPointCollection*> const& collections);

        //void setDefinitionMap(std::map<std::string,DataPointDefinition*> *defMap) {defMap_=defMap;}

        const DataPoint * getDataAt(size_t index) const {
          if (index>=data_.size()) return nullptr;
          return &data_[index];
        }

        void reset();

        bool isInitialized() {return isInitialized_;}

	// JSON field names
	static const std::string SOURCE;
	static const std::string DEFINITION;
	static const std::string DATA;

protected:
	//for simple usage
	std::string filename_;
	std::string source_;
	std::string *sourcePtr_;
	std::string definition_;
        std::map<std::string,DataPointDefinition*> *defMap_=nullptr;
        DataPointDefinition *def_=nullptr;

	std::vector<DataPoint> data_;
        std::vector<TrackedMonitorable> *trackedVector_ = nullptr;
        bool fastMon_=false;
        bool isInitialized_=false;


};
}

#endif /* DATAPOINTCOLLECTION_H_ */
