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
  if (!pObj->serialize(serializeRoot))
    return false;

  Json::StyledWriter writer;
  output = writer.write(serializeRoot);

  return true;
}

bool JSONSerializer::deserialize(JsonSerializable* pObj, std::string & input)
{
  assert(pObj!=nullptr);

  Json::Value deserializeRoot;
  Json::Reader reader;

  if (!reader.parse(input, deserializeRoot))
    return false;


  if (!pObj->deserialize(deserializeRoot))
    return false;

  return true;
}

