
#include "EventFilter/Utilities/interface/JsonMonitorable.h"

//asserts if combination of operation, monitorable type and JsonMonitorable is now allowed

using namespace jsoncollector;

JsonMonitorable* IntJ::clone() {
  IntJ* newVar = new IntJ();
  *newVar = *this;
  return newVar;
}

JsonMonitorable* IntJ::cloneType() {
  return new IntJ();
}


JsonMonitorable* DoubleJ::clone() {
  DoubleJ* newVar = new DoubleJ();
  *newVar = *this;
  return newVar;
}

JsonMonitorable* DoubleJ::cloneType() {
  return new DoubleJ();
}


JsonMonitorable* StringJ::clone() {
  StringJ* newVar = new StringJ();
  *newVar = *this;
  return newVar;
}

JsonMonitorable* StringJ::cloneType() {
  return new StringJ();
}


template <class T,class T2> JsonMonitorable* VectorJ<T,T2>::clone() {
  VectorJ<T,T2>* newVar = new VectorJ<T,T2>(expectedUpdates_,maxUpdates_);
  *newVar = *this;
  return newVar;
}
template JsonMonitorable* VectorJ<long,IntJ>::clone();
template JsonMonitorable* VectorJ<double,DoubleJ>::clone();
template JsonMonitorable* VectorJ<std::string,StringJ>::clone();
/*
template <class T,class T2> JsonMonitorable* VectorJ<T,T2>::cloneType() {
  return new VectorJ<T,T2>(expectedUpdates_,maxUpdates_);
}
template JsonMonitorable* VectorJ<long,IntJ>::cloneType();
template JsonMonitorable* VectorJ<double,DoubleJ>::cloneType();
template JsonMonitorable* VectorJ<std::string,StringJ>::cloneType();
*/

template <class T,class T2> JsonMonitorable* HistoJ<T,T2>::clone() {
  HistoJ<T,T2>* newVar = new HistoJ<T,T2>(len_);
  *newVar = *this;
  return newVar;
}
template JsonMonitorable* HistoJ<long,IntJ>::clone();

/*
template <class T,class T2> JsonMonitorable* HistoJ<T,T2>::cloneType() {
  return new HistoJ<T,T2>(len_);
}
template JsonMonitorable* HistoJ<long,IntJ>::cloneType();
*/

void JsonMonitorable::typeCheck(MonType type, OperationType op, SnapshotMode sm, JsonMonitorable * mon)
{
  assert(OP_UNKNOWN);
  if (dynamic_cast<IntJ*>(mon)) {
    assert(type==TYPE_INT);
    assert(op==OP_SUM || op==OP_AVG || op== OP_SAME || op== OP_SAMPLE || op == OP_BINARYAND || op == OP_BINARYOR || op == OP_ADLER32 || ((op==OP_APPEND || op==OP_HISTO) && sm==SM_TIMER ));
  }
  else if (dynamic_cast<DoubleJ*>(mon)) {
    assert(type==TYPE_DOUBLE);
    assert(op==OP_SUM || op==OP_AVG || op== OP_SAME || op== OP_SAMPLE || (op==OP_APPEND && sm==SM_TIMER));
  }
  else if (dynamic_cast<StringJ*>(mon)) {
    assert(type==TYPE_STRING);
    assert(op==OP_SAME || op==OP_SAMPLE || op==OP_CAT || (op==OP_APPEND && sm==SM_TIMER));
  }
  else if (dynamic_cast<VectorJ<long,IntJ>*>(mon)) {
    assert(type==TYPE_INT);
    assert(op==OP_APPEND);
    assert(sm==SM_EOL);
  }
  else if (dynamic_cast<VectorJ<double,DoubleJ>*>(mon)) {
    assert(type==TYPE_DOUBLE);
    assert(op==OP_APPEND);
    assert(sm==SM_EOL);
  }
  else if (dynamic_cast<VectorJ<std::string,StringJ>*>(mon)) {
    assert(type==TYPE_STRING);
    assert(op==OP_APPEND);
    assert(sm==SM_EOL);
  }
  else if (dynamic_cast<HistoJ<long,IntJ>*>(mon)) {
    assert(type==TYPE_INT);
    assert(op==OP_HISTO);
    assert(sm==SM_EOL);
  }
  else assert(0);
/*  else if (dynamic_cast<HistoJ<DoubleJ>>(mon)) {
    assert(0);//currently unsupported
    //assert(type==TYPE_DOUBLE);
    //assert(op==OP_HISTO);
    //assert(sm!=SM_TIMER);
  }
  else if (dynamic_cast<HistoJ<StringJ>>(mon)) {assert(0);}
*/
}

//for validation of deserialized content
bool JsonMonitorable::typeAndOperationCheck(MonType type, OperationType op)
{
  switch (op) {
    case OP_CAT:
      if (type==TYPE_STRING) return true;
      break;
    case OP_SUM:
    case OP_AVG:
    case OP_HISTO:
      if (type==TYPE_INT || type==TYPE_DOUBLE) return true;
      break;
    case OP_APPEND:
    case OP_SAME:
    case OP_SAMPLE:
      return true;
    case OP_BINARYAND:
    case OP_BINARYOR:
    case OP_ADLER32:
      if (type==TYPE_INT) return true;
    case OP_UNKNOWN:
    default:
      break;
  }
  return false;
}



JsonMonitorable* JsonMonitorable::createMonitorable(Json::Value const& data, MonType type, OperationType op)
{
  JsonMonitorable *obj;
  switch (type) {
    case TYPE_INT:
      switch (op) {
        case OP_SUM:
        case OP_AVG:
        case OP_SAME:
        case OP_SAMPLE:
        case OP_BINARYAND:
        case OP_BINARYOR:
          obj = new IntJ();
          if (((IntJ*)obj)->parse(data)) return obj;
          else delete (IntJ*)obj;
        case OP_HISTO:
          if (!data.isArray()) return nullptr;
          obj = new HistoJ<long,IntJ>(0);
          for (size_t i=0;i<data.size();i++) {
           if (data[i].isString()) {
             try {
               ((HistoJ<long,IntJ>*)obj)->append(boost::lexical_cast<long>(data[i].asString()));
             } catch(...) { delete (HistoJ<long,IntJ>*)obj;return nullptr;}
           }
           else
             ((HistoJ<long,IntJ>*)obj)->append(data[i].asInt());
          }
          return obj;
          break;
        case OP_APPEND:
          if (!data.isArray()) return nullptr;
          obj = new VectorJ<long,IntJ>(0,JSON_DEFAULT_MAXUPDATES);
          for (size_t i=0;i<data.size();i++) {
            if (data[i].isString()) {
             try {
                if (!((VectorJ<long,IntJ>*)obj)->append(boost::lexical_cast<long>(data[i].asString()))) {
                  delete (VectorJ<long,IntJ>*)obj;
                  return nullptr;
                }
              } catch (...) { delete (VectorJ<long,IntJ>*)obj;return nullptr;}
            }
            else {
              if (!((VectorJ<long,IntJ>*)obj)->append(data[i].asInt())) {
                delete (VectorJ<long,IntJ>*)obj;
                return nullptr;
              }
            }
          }
          return obj;
        default:
          break;
        }
        break;
    case TYPE_DOUBLE:
      switch (op) {
        case OP_SUM:
        case OP_AVG:
        case OP_SAME:
        case OP_SAMPLE:
          obj = new DoubleJ();
          if (((DoubleJ*)obj)->parse(data)) return obj;
          else delete (DoubleJ*)obj;
          break;
        case OP_APPEND:
          if (!data.isArray()) return nullptr;
          obj = new VectorJ<double,DoubleJ>(0,JSON_DEFAULT_MAXUPDATES);

          for (size_t i=0;i<data.size();i++) {
            if (data[i].isString()) {
              try {
                if (!((VectorJ<double,DoubleJ>*)obj)->append(boost::lexical_cast<double>(data[i].asString()))) {
                  delete (VectorJ<double,DoubleJ>*)obj;
                  return nullptr;
                }
              } catch(...) { delete (VectorJ<double,DoubleJ>*)obj;return nullptr;}
            }
            else{
              if (!((VectorJ<double,DoubleJ>*)obj)->append(data[i].asDouble())) {
                delete (VectorJ<double,DoubleJ>*)obj;
                return nullptr;
              }
            }
          }
          return obj;
        default:
          break;
        }
      break;
    case TYPE_STRING:
      switch (op) {
        case OP_SAME:
        case OP_SAMPLE:
        case OP_CAT:
          obj = new StringJ();
          if (((StringJ*)obj)->parse(data)) return obj;
          else delete (StringJ*) obj;
        case OP_APPEND:
          if (!data.isArray()) return nullptr;
          obj = new VectorJ<std::string,StringJ>(0,JSON_DEFAULT_MAXUPDATES);
          for (size_t i=0;i<data.size();i++) {
            if (!data[i].isString()) {delete (VectorJ<std::string,StringJ>*)obj;return nullptr;}
            if (!((VectorJ<std::string,StringJ>*)obj)->append(data[i].asString())) {
              delete (VectorJ<std::string,StringJ>*)obj;
              return nullptr;
            }
          }
          return obj;
        default:
          break;
        }
    default:
      return nullptr;
  }
  return nullptr;
}











bool JsonMonitorable::mergeData(JsonMonitorable *monitorable, std::vector<JsonMonitorable*>* vec,MonType type, OperationType op,bool final)
{

  if (!vec->size()) {
    monitorable->resetValue();
    return false;
  }

  switch (type) {
    case TYPE_INT:
        switch (op) {
            case OP_SUM:
                for (auto const& v: *vec) static_cast<IntJ*>(monitorable)->addVal(static_cast<IntJ*>(v)->value());
                break;
            case OP_AVG:
                for (auto const& v: *vec) static_cast<IntJ*>(monitorable)->addVal(static_cast<IntJ*>(v)->value());
                if (final) static_cast<IntJ*>(monitorable)->normalize();
                break;
            case OP_SAME:
                for (auto const& v: *vec) static_cast<IntJ*>(monitorable)->update((IntJ*)v);
                break;
            case OP_SAMPLE:
                if (vec->size()) static_cast<IntJ*>(monitorable)->update((IntJ*)(vec->at(vec->size()-1U)));//take last
                break;
            case OP_HISTO:
                for (auto const& v: *vec) { if (!static_cast<HistoJ<long,IntJ>*>(monitorable)->updateWithHisto(static_cast<HistoJ<long,IntJ>*>(v))) return false;}
                break;
            case OP_APPEND:
                for (auto const& v: *vec) static_cast<VectorJ<long,IntJ>*>(monitorable)->updateWithVector((VectorJ<long,IntJ>*)v);
                break;
            case OP_BINARYAND:
                for (auto const& v: *vec) static_cast<IntJ*>(monitorable)->binaryAnd((IntJ*)v);
                break;
            case OP_BINARYOR:
                for (auto const& v: *vec) static_cast<IntJ*>(monitorable)->binaryOr((IntJ*)v);
                break;
            case OP_ADLER32:
                //for (v:*vec) static_cast<IntJ>*(monitorable)->update(v);//combination not handled yet
                break;
            default:
                return false;
        }
        break;

    case TYPE_DOUBLE:
        switch (op) {
            case OP_SUM:
                for (auto const& v: *vec) static_cast<DoubleJ*>(monitorable)->addVal(static_cast<DoubleJ*>(v)->value());
                break;
            case OP_AVG:
                for (auto const& v: *vec) static_cast<DoubleJ*>(monitorable)->addVal(static_cast<DoubleJ*>(v)->value());
                if (final) static_cast<DoubleJ*>(monitorable)->normalize();
                break;
            case OP_SAME:
                for (auto const& v: *vec) static_cast<DoubleJ*>(monitorable)->update((DoubleJ*)v);
                break;
            case OP_SAMPLE:
                if (vec->size()) static_cast<DoubleJ*>(monitorable)->update((DoubleJ*)(vec->at(vec->size()-1U)));//take last
                break;
            case OP_APPEND:
                for (auto const& v: *vec) static_cast<VectorJ<double,DoubleJ>*>(monitorable)->updateWithVector((VectorJ<double,DoubleJ>*)v);
                break;
            default:
                return false;
        }
        break;

    case TYPE_STRING:
        switch (op) {
            case OP_SAME:
                for (auto const& v:*vec) static_cast<StringJ*>(monitorable)->update((StringJ*)v);
                break;
            case OP_SAMPLE:
                if (vec->size()) static_cast<StringJ*>(monitorable)->update((StringJ*)(vec->at(vec->size()-1U)));//take last
                break;
            case OP_APPEND:
                for (auto const& v:*vec) static_cast<VectorJ<std::string,StringJ>*>(monitorable)->updateWithVector((VectorJ<std::string,StringJ>*)v);
                break;
            case OP_CAT:
                for (auto const& v:*vec) static_cast<StringJ*>(monitorable)->concatenate((StringJ*)v);
                break;
            default:
                return false;
        }
        break;
    default:
        return false;
    }
    return true;
  }


//initialize aggregation vector types for certain operations in timer mode
JsonMonitorable* IntJ::cloneAggregationType(OperationType op, SnapshotMode sm, size_t bins, size_t expectedUpdates, size_t maxUpdates) {
  switch (sm) {
    case SM_TIMER:
      if (op==OP_CAT || op==OP_APPEND)
	return new VectorJ<long,IntJ>(expectedUpdates,maxUpdates);
      if (op==OP_HISTO)
	return new HistoJ<long,IntJ>(bins);
    default:
      return new IntJ(theVar_);
  }
}

JsonMonitorable* DoubleJ::cloneAggregationType(OperationType op, SnapshotMode sm, size_t bins, size_t expectedUpdates, size_t maxUpdates) {
  switch (sm) {
    case SM_TIMER:
      if (op==OP_CAT || op==OP_APPEND)
	return new VectorJ<double,DoubleJ>(expectedUpdates,maxUpdates);
      if (op==OP_HISTO)
	return new HistoJ<double,DoubleJ>(bins);
    default:
      return new DoubleJ(theVar_);
  }
}

JsonMonitorable* StringJ::cloneAggregationType(OperationType op, SnapshotMode sm, size_t bins, size_t expectedUpdates, size_t maxUpdates) {
  switch (sm) {
    case SM_TIMER:
      if (op==OP_CAT || op==OP_APPEND)
	return new VectorJ<std::string,StringJ>(expectedUpdates,maxUpdates);
    default:
      return new StringJ(theVar_);
  }
}

