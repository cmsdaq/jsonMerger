/*
 * DataPointDefinition.cc
 *
 *  Created on: Sep 24, 2012
 *      Author: aspataru
 */

#include <sys/stat.h>

#include "EventFilter/Utilities/interface/DataPointDefinition.h"
#include "EventFilter/Utilities/interface/JsonMonitorable.h"
#include "EventFilter/Utilities/interface/FileIO.h"
#include "EventFilter/Utilities/interface/JSONSerializer.h"
//#include "FWCore/MessageLogger/interface/MessageLogger.h"

#include <fstream>

using namespace jsoncollector;

const std::string DataPointDefinition::LEGEND = "legend";
const std::string DataPointDefinition::DATA = "data";
const std::string DataPointDefinition::PARAM_NAME = "name";
const std::string DataPointDefinition::OPERATION = "operation";
const std::string DataPointDefinition::TYPE = "type";

const char* DataPointDefinition::typeNames_[] = {"null","integer","double","string"};
const char* DataPointDefinition::operationNames_[] = {"null","sum","avg","same","sample","histo","append","cat","binaryAnd","binaryOr","adler32"};

DataPointDefinition::DataPointDefinition() {
 for (unsigned int i=0;i<sizeof(typeNames_)/sizeof(char*);i++)
   typeMap_[typeNames_[i]]=(MonType)i;
 for (unsigned int i=0;i<sizeof(operationNames_)/sizeof(char*);i++)
   operationMap_[operationNames_[i]]=(OperationType)i;
}

//static member implementation
//
std::string DataPointDefinition::findDefinitionFile(const std::string *defaultDirectory,std::string def)
{
  std::string fullpath;
  while (def.size()) {
    if (def.find('/')==0)
      fullpath = def;
    else {
      if (defaultDirectory)
        fullpath = *defaultDirectory+'/'+def; 
      else 
        fullpath = def;//TODO:get working directory
    }
    struct stat buf;
    if (stat(fullpath.c_str(), &buf) == 0) {
      break;
    }
    //check if we can still find definition
    if (def.size()<=1 || def.find('/')==std::string::npos) {
      return std::string();
    }
    def = def.substr(def.find('/')+1);
  }
  return fullpath;
}
std::string DataPointDefinition::definitionPathWithoutPid(std::string const& defFilePath)
{

  std::string defFilePathWithoutPid;
  auto pidPos =  defFilePath.rfind("_pid");

  //no _pid or _pid in directory name
  if (pidPos == std::string::npos || defFilePath.substr(pidPos,std::string::npos).find('/')!=std::string::npos) {
    defFilePathWithoutPid=defFilePath;
  }
  else {
    //_pid in name
    defFilePathWithoutPid=defFilePath.substr(0,pidPos);
    std::string defFileAfterExt = defFilePath.substr(defFilePath.rfind("."),std::string::npos);
    if (defFileAfterExt.size() && defFileAfterExt.rfind("/")==std::string::npos && defFileAfterExt.rfind("_pid")==std::string::npos)
      defFilePathWithoutPid += defFileAfterExt;
  }
  return defFilePathWithoutPid;
}

bool DataPointDefinition::getDataPointDefinitionFor(std::string const& defFilePath,
                                                    DataPointDefinition* dpd,
                                                    const std::string *defaultGroup,
                                                    std::map<std::string,DataPointDefinition*> *defMap,
                                                    const std::string * defaultDirectory,
                                                    bool copyDefinitionMaybe)
{

  //todo:see hwo this will work in CMSSW

  std::string defFilePathWithoutPid = definitionPathWithoutPid(defFilePath);
  std::string def = findDefinitionFile(defaultDirectory,defFilePath);

  std::string dpdString;
  bool readOK=false;
  if (def!=std::string())
    readOK = FileIO::readStringFromFile(def, dpdString);
  if (!readOK) {
    def = findDefinitionFile(defaultDirectory,defFilePathWithoutPid);
    if (def==std::string()) return false;
    readOK = FileIO::readStringFromFile(def, dpdString);
  }
  else if (copyDefinitionMaybe) {
    struct stat buf;
    std::string destWithoutPid = definitionPathWithoutPid(def);
    if (stat(destWithoutPid.c_str(), &buf) != 0) {
     std::ifstream  src(def, std::ios::binary);
     std::ofstream  dst(destWithoutPid,   std::ios::binary);
     dst << src.rdbuf();
    }
  }
  if (!readOK) return false;
  
  if (defaultGroup and defaultGroup->size()) dpd->setDefaultGroup(*defaultGroup);
  dpd->setDefFilePath(defFilePathWithoutPid);


  bool parseOK = JSONSerializer::deserialize(dpd, dpdString);
  if (!parseOK) return false;


//  TODO: CMSSW
//  try {
//    bool parseOK = JSONSerializer::deserialize(dpd, dpdString);
//    if (!parseOK) return false;
//  }
//  catch (std::exception &e) {
//    //edm::LogWarning("DataPointDefinition") << e.what() << " file -: " << defFilePath;
//    return false;
//  }
//

  if (defMap) (*defMap)[defFilePathWithoutPid]=dpd; 
  return true;
}

bool DataPointDefinition::serialize(Json::Value& root)
{

  for (unsigned int i = 0; i < variables_.size(); i++) {
    Json::Value currentDef;
    currentDef[PARAM_NAME] = variables_[i].getName();
    if (variables_[i].getType()!=TYPE_UNKNOWN) //only if type is known
      currentDef[TYPE] = typeNames_[variables_[i].getType()];
    currentDef[OPERATION] = operationNames_[variables_[i].getOperation()];
    root[defaultGroup_].append(currentDef);
  }
  return true;
}

bool DataPointDefinition::deserialize(Json::Value& root)
{
  bool res = true;
  if (!defaultGroup_.size() || root.get(defaultGroup_,"").asString()=="") {
    //detect if definition is specified with "data" or "legend"
    if (root.get(LEGEND,"").isArray() ) defaultGroup_=LEGEND;
    else if (root.get(DATA,"").isArray()) defaultGroup_=DATA;
    //else throw exception
  }


  if (root.get(defaultGroup_, "").isArray()) {
    unsigned int size = root.get(defaultGroup_, "").size();
    for (unsigned int i = 0; i < size; i++) {
      bool defValid = addLegendItem(root.get(defaultGroup_, "")[i].get(PARAM_NAME, "").asString(),
                               root.get(defaultGroup_, "")[i].get(TYPE, "").asString(),
                               root.get(defaultGroup_, "")[i].get(OPERATION, "").asString());
      if (!defValid) res=false;
    }
  }
  else res=false;

  return res;
//TODO:CMSSW
//  if (!res)
//    throw std::exception("Invalid definition file");
}

bool DataPointDefinition::isPopulated() const
{
  if (variables_.size() > 0) return true;
  else return false;
}

bool DataPointDefinition::hasVariable(std::string const&name, size_t *index)
{
  for (size_t i=0;i<variables_.size();i++)
    if (variables_[i].getName()==name) {
      if (index)
        *index = i;
      return true;
    }
  return false;
}

MonType DataPointDefinition::getType(unsigned int index)
{
  if (index>=variables_.size()) return TYPE_UNKNOWN;
  return variables_[index].getType();
}


OperationType DataPointDefinition::getOperation(unsigned int index)
{
  if (index>=variables_.size()) return OP_UNKNOWN;
  return variables_[index].getOperation();
}

bool DataPointDefinition::addLegendItem(std::string const& name, std::string const& type, std::string const& operation, bool isValidated, EmptyMode em, SnapshotMode sm)
{

  if (!defaultGroup_.size()) defaultGroup_=DATA;

  bool ret = true;
  auto typeItr = typeMap_.find(type);
  auto opItr = operationMap_.find(operation);
  if (opItr==operationMap_.end() || operation=="null") {
    opItr=operationMap_.find("null");
    ret=false;
  }
  if (typeItr==typeMap_.end()) {
    typeItr=typeMap_.find("null");
    ret=false;
  }
  variables_.emplace_back(name,typeItr->second,opItr->second,isValidated,em,sm);
  nameMap_[name]=variables_.size()-1;
  return ret;
}

void DataPointDefinition::addMonitorableDefinition(MonitorableDefinition & monDef) {
  variables_.push_back(monDef);
}

MonitorableDefinition *DataPointDefinition::getMonitorableDefinition(std::string const& name) {
  auto itr = nameMap_.find(name);
  if (itr!=nameMap_.end()) return &(variables_.at(itr->second));
  else return nullptr;
}


MonitorableDefinition *DataPointDefinition::getMonitorableDefinition(size_t index) {
  if (index>=variables_.size()) return nullptr;
  return &(variables_.at(index));
}



