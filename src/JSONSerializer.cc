/*
 * JSONSerializer.cc
 *
 *  Created on: Aug 2, 2012
 *      Author: aspataru
 */

#include "EventFilter/Utilities/interface/JSONSerializer.h"

#include <assert.h>

using namespace jsoncollector;

bool JSONSerializer::serialize(JsonSerializable* pObj, std::string & output)
{
  assert(pObj!=nullptr);

  Json::Value serializeRoot;
  if (!pObj->serialize(serializeRoot)) {
    std::cout << "File serialization failed!" << std::endl;
    return false;
  }

  Json::StyledWriter writer;
  output = writer.write(serializeRoot);

  return true;
}

bool JSONSerializer::deserialize(JsonSerializable* pObj, std::string & input)
{
  assert(pObj!=nullptr);

  Json::Value deserializeRoot;
  Json::Reader reader;

  if (!reader.parse(input, deserializeRoot)) {
    std::cout << "File parsing failed!" << std::endl;
    return false;
  }


  if (!pObj->deserialize(deserializeRoot)) {
    std::cout << "File deserialization failed!" << std::endl;
    return false;
  }

  return true;
}

