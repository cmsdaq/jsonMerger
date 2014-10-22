/*
 * JsonMonitorable.h
 *
 *  Created on: Oct 29, 2012
 *      Author: aspataru
 */

#ifndef JSON_MONITORABLE_H
#define JSON_MONITORABLE_H

#include "json.h"

#include <string>
#include <sstream>
#include <vector>
#include <memory>
#include <iostream>

#include <zlib.h>
#include <assert.h>

#include <boost/algorithm/string.hpp>
#include <boost/lexical_cast.hpp>

#define JSON_DEFAULT_MAXUPDATES 100

namespace jsoncollector {

enum MonType  { TYPE_UNKNOWN, TYPE_INT, TYPE_DOUBLE, TYPE_STRING };

enum OperationType  { OP_UNKNOWN, OP_SUM, OP_AVG, OP_SAME, OP_SAMPLE, OP_HISTO, OP_APPEND, OP_CAT, OP_BINARYAND, OP_BINARYOR, OP_ADLER32 };

enum EmptyMode  { EM_UNSET, EM_NA, EM_EMPTY };

enum SnapshotMode  { SM_UNSET, SM_TIMER, SM_EOL };

//TODO:resolve updates_ policy

class JsonMonitorable {

public:

	JsonMonitorable() : updates_(0), notSame_(false) {}

	virtual ~JsonMonitorable() {}

        virtual JsonMonitorable* clone() = 0;

        virtual JsonMonitorable* cloneType() = 0;

        virtual JsonMonitorable* cloneAggregationType(OperationType op, SnapshotMode sm, size_t bins=0, size_t expectedUpdates=0, size_t maxUpdates=0) = 0;

	virtual std::string toString() const = 0;

	virtual Json::Value toJsonValue() const  = 0;

	virtual void resetValue() = 0;

        virtual bool update(JsonMonitorable* mon,SnapshotMode sm, OperationType op) = 0;

	unsigned int getUpdates() {return updates_;}

        static void typeCheck(MonType type, OperationType op, SnapshotMode sm, JsonMonitorable * mon);
        static bool typeAndOperationCheck(MonType type, OperationType op);
        static bool mergeData(JsonMonitorable *monitorable, std::vector<JsonMonitorable*>* vec,MonType type, OperationType op,bool final=true);

        static JsonMonitorable* createMonitorable(Json::Value const& data, MonType type, OperationType op); 

	void resetUpdates() {updates_=0;notSame_=false;}

	bool getNotSame() {return notSame_;}

protected:
	unsigned int updates_;
	bool notSame_;
};

//unused..
class JsonMonPtr {
public:
	JsonMonPtr():ptr_(nullptr){}
	JsonMonPtr(JsonMonitorable*ptr):ptr_(ptr){}
	void operator=(JsonMonitorable* ptr ){ptr_=ptr;}
	~JsonMonPtr() {if (ptr_) delete ptr_;ptr_=nullptr;}
	JsonMonitorable* operator->() {return ptr_;}
	JsonMonitorable* get() {return ptr_;}
	//JsonMonPtr& operator=(JsonMonPtr& ) = delete;
	//JsonMonPtr& operator=(JsonMonPtr&& other){ptr_=other.ptr_;return *this;}
private:
	JsonMonitorable *ptr_;
};


class IntJ;

class IntJ: public JsonMonitorable {

public:
	IntJ() : JsonMonitorable(), theVar_(0) {}
	IntJ(long val) : JsonMonitorable(), theVar_(val) {}

	virtual ~IntJ() {}

        virtual JsonMonitorable* clone();

        virtual JsonMonitorable* cloneType();

        virtual JsonMonitorable* cloneAggregationType(OperationType op, SnapshotMode sm, size_t bins=0, size_t expectedUpdates=0, size_t maxUpdates=0);

	virtual std::string toString() const {
		std::stringstream ss;
		ss << theVar_;
		return ss.str();
	}

	virtual Json::Value toJsonValue() const {
                Json::Value val = theVar_;
                return val;
	}

	virtual void resetValue() {
		theVar_=0;
		updates_=0;
		notSame_=0;
	}
        virtual bool update(JsonMonitorable* mon,SnapshotMode sm, OperationType op)
        {
                switch (op) {
                  case OP_SUM:
                  case OP_AVG:
                    add((static_cast<IntJ*>(mon)));
                    break;
                  case OP_SAME:
                  case OP_SAMPLE:
                    update((static_cast<IntJ*>(mon)));
                    break;
                   //not yet in SM_TIMER mode
                  case OP_BINARYOR:
                    binaryOr(static_cast<IntJ*>(mon));
                    break;
                  case OP_BINARYAND:
                    binaryAnd(static_cast<IntJ*>(mon));
                    break;
                  default:
                    assert(0);
                }
                return true;
        }

	void operator=(long sth) {
		theVar_ = sth;
		updates_=1;
		notSame_=0;
	}
	long & value() {
		return theVar_;
	}

	void updateVal(long l) {
		if (updates_ && theVar_!=l) notSame_=true;
		theVar_=l;
		updates_++;
	}

	void update(IntJ* mon) {
		if (updates_ && theVar_!=mon->value()) notSame_=true;
		theVar_=mon->value();
		updates_++;
	}

	void addVal(long l) {
                if (!updates_) theVar_=l;
		else theVar_+=l;
		updates_++;
	}

	void add(IntJ* mon) {
		if (updates_ && theVar_!=mon->value()) notSame_=true;
                if (!updates_) theVar_=mon->value();
		else theVar_=+mon->value();
		updates_+=mon->getUpdates();
	}


	void binaryAndVal(long l) {
                if (updates_) theVar_=theVar_ & l;
                else theVar_= l;
                updates_++;
        }

	void binaryAnd(IntJ* mon) {
                if (mon->getUpdates() && updates_) theVar_=theVar_ & mon->value();
                else if (mon->getUpdates()) theVar_= mon->value();
                else return;
                updates_+=mon->getUpdates();
        }

	void binaryOrVal(long l) {
                if (!updates_) theVar_ = l;
                else theVar_ = theVar_ | l;
                updates_++;
        }

	void binaryOr(IntJ* mon) {
                if (!updates_) theVar_ = mon->value();
                else theVar_=theVar_ | mon->value();
                updates_+=mon->getUpdates();
        }

        void normalize() {
          if (updates_) {
            theVar_/=updates_;
            updates_=1;
          }
        }

        bool parse(std::string const& s) {
          try {
            theVar_ = boost::lexical_cast<long>(s);
            updates_=1;
          }
          catch (...) {return false;}
          return true;
        }


        bool parse(Json::Value const& s) {
          try {
            theVar_ = s.asInt();
            updates_=1;
          }
          catch (...) {return false;}
          return true;
        }


protected:
	long theVar_;
};

class IntJAdler32: public IntJ {
public:

	IntJAdler32(long adler32, long len):
          IntJ(adler32),
          len_(len) {}

        long len() {return len_;}

	void adler32combine(IntJAdler32* mon) {
                if (!mon || !mon->len() || mon->value()<0) return;
                value()=adler32_combine(theVar_,mon->len(),mon->value()&0xffffffff);
                len_+=mon->len();
	};
private:
        long len_;
};

class DoubleJ: public JsonMonitorable {

public:
	DoubleJ() : JsonMonitorable(), theVar_(0) {}
	DoubleJ(double val) : JsonMonitorable(), theVar_(val) {}

	virtual ~DoubleJ() {}

        virtual JsonMonitorable* clone();

        virtual JsonMonitorable* cloneType();

        virtual JsonMonitorable* cloneAggregationType(OperationType op, SnapshotMode sm, size_t bins=0, size_t expectedUpdates=0, size_t maxUpdates=0);

	virtual std::string toString() const {
		std::stringstream ss;
		ss << theVar_;
		return ss.str();
	}

	virtual Json::Value toJsonValue() const {
                Json::Value val = theVar_;
                return val;
	}

	virtual void resetValue() {
		theVar_=0;
		updates_=0;
		notSame_=0;
	}

        virtual bool update(JsonMonitorable* mon,SnapshotMode sm, OperationType op)
        {
                switch (op) {
                  case OP_SUM:
                  case OP_AVG:
                    add((static_cast<DoubleJ*>(mon)));
                    break;
                  case OP_SAME:
                  case OP_SAMPLE:
                    update((static_cast<DoubleJ*>(mon)));
                    break;
                  default:
                    assert(0);
                }
                return true;
        }

	void operator=(double sth) {
		theVar_ = sth;
		updates_=1;
		notSame_=0;
	}

	double & value() {
		return theVar_;
	}
	void updateVal(double sth) {
		if (updates_ && theVar_!=sth) notSame_=true;
		theVar_=sth;
		updates_++;
	}

	void update(DoubleJ * s) {
		if (updates_ && theVar_!=s->value()) notSame_=true;
		theVar_=s->value();
		updates_++;
	}

	void addVal(double d ) {
		theVar_+=d;
		updates_++;
	}

	void add(DoubleJ * d) {
		theVar_+=d->value();
		updates_++;
	}

        void normalize() {
          if (updates_) {
            theVar_/=updates_;
            updates_=1;
          }
        }


        bool parse(std::string const& s) {
          try {
            theVar_ = boost::lexical_cast<double>(s);
            updates_=1;
          }
          catch (...) {return false;}
          return true;
        }

        bool parse(Json::Value const& s) {
          try {
            theVar_ = s.asDouble();
            updates_=1;
          }
          catch (...) {return false;}
          return true;
        }


private:
	double theVar_;
};


class StringJ: public JsonMonitorable {

public:
	StringJ() :  JsonMonitorable() {}
	StringJ(std::string val) : JsonMonitorable(),theVar_(val) {}

	virtual ~StringJ() {}

        virtual JsonMonitorable* clone();

        virtual JsonMonitorable* cloneType();

        virtual JsonMonitorable* cloneAggregationType(OperationType op, SnapshotMode sm, size_t bins=0, size_t expectedUpdates=0, size_t maxUpdates=0);

        virtual bool update(JsonMonitorable* mon,SnapshotMode sm, OperationType op)
        {
                switch (op) {
                  case OP_SAME:
                  case OP_SAMPLE:
                    update(static_cast<StringJ*>(mon));
                    break;
                  case OP_CAT:
                    concatenate(static_cast<StringJ*>(mon));
                    break;
                  default:
                    assert(0);
                }
                return true;
        }

	virtual std::string toString() const {
		return theVar_;
	}

	virtual Json::Value toJsonValue() const {
                Json::Value val = theVar_;
                return val;
	}

	virtual void resetValue() {
		theVar_=std::string();
		updates_ = 0;
		notSame_=false;
	}

	void operator=(std::string const& sth) {
		theVar_ = sth;
		updates_=1;
		notSame_=false;
	}

	std::string const& value() {
		return theVar_;
	}

	void concatenateVal(std::string const& added) {
		if (!updates_)
		  theVar_=added;
		else
                  if (added.size())
		    theVar_+=","+added;
		updates_++;
	}

	void concatenate(StringJ* s) {
		if (!updates_)
		  theVar_=s->value();
		else
                  if (s->value().size())
		    theVar_+=","+s->value();
		updates_+=s->getUpdates();
	}

	void updateVal(std::string const& newStr) {
		if (updates_ && theVar_!=newStr) notSame_=true;
		theVar_=newStr;
		updates_=1;
	}

	void update(StringJ* s) {
		if (updates_ && theVar_!=s->value()) notSame_=true;
		theVar_=s->value();
		updates_=1;
	}

        bool parse(std::string const& s) {
          theVar_ = s;
          updates_=1;
          return true;
        }


        bool parse(Json::Value const& s) {
          theVar_ = s.asString();
          updates_=1;
          return true;
        }


private:
	std::string theVar_;
};

//vectors filled at time intervals
template<class T,class T2> class VectorJ: public JsonMonitorable {

public:
	VectorJ( int expectedUpdates = 1 , unsigned int maxUpdates = 0 ): JsonMonitorable() {
		expectedUpdates_=expectedUpdates;
		maxUpdates_ = maxUpdates;
		if (maxUpdates_ && maxUpdates_<expectedUpdates_) expectedUpdates_=maxUpdates_;
		vec_.reserve(expectedUpdates_);
	}
	virtual ~VectorJ() {}

        virtual JsonMonitorable* clone();

        virtual JsonMonitorable* cloneType() {
            return new VectorJ<T,T2>(expectedUpdates_,maxUpdates_);
        }

        virtual JsonMonitorable* cloneAggregationType(OperationType op, SnapshotMode sm, size_t bins=0, size_t expectedUpdates=0, size_t maxUpdates=0) {
          return cloneType();
        }

	virtual std::string toString() const {
		std::stringstream ss;
		ss << "[";
		if (vec_.size())
			for (unsigned int i = 0; i < vec_.size(); i++) {
				ss << vec_[i];
				if (i<vec_.size()-1) ss << ",";
			}
		ss << "]";
		return ss.str();
	}

	virtual Json::Value toJsonValue() const {
                Json::Value val(Json::arrayValue);
		for (auto& v: vec_) val.append(Json::Value(v));
                return val;
	}

	virtual void resetValue() {
		vec_.clear();
		vec_.reserve(expectedUpdates_);
		updates_=0;
	}


        virtual bool update(JsonMonitorable* mon,SnapshotMode sm, OperationType op)
        {
                if (sm==SM_TIMER) {
                  if (op==OP_CAT || op==OP_APPEND) updateWithScalar(static_cast<T2*>(mon));
                  else assert(0);
                }
                else updateWithVector(static_cast<VectorJ<T,T2>*>(mon));
                 return true;
        }

        void updateWithScalar(T2* t) {
          //TODO
        }
        void updateWithVector(VectorJ<T,T2>* t) {
          //TODO
        }

	void operator=(std::vector<T> & sth) {
		vec_ = sth;
	}

	std::vector<T> & value() {
		return vec_;
	}

	unsigned int getExpectedUpdates() {
		return expectedUpdates_;
	}

	unsigned int getMaxUpdates() {
		return maxUpdates_;
	}

	void setMaxUpdates(unsigned int maxUpdates) {
		maxUpdates_=maxUpdates;
		if (!maxUpdates_) return;
		if (expectedUpdates_>maxUpdates_) expectedUpdates_=maxUpdates_;
		//truncate what is over the limit
		if (maxUpdates_ && vec_.size()>maxUpdates_) {
			vec_.resize(maxUpdates_);
		}
		else vec_.reserve(expectedUpdates_);
	}

	unsigned int getSize() {
		return vec_.size();
	}

	void update(T val) {
		if (maxUpdates_ && updates_>=maxUpdates_) return;
		vec_.push_back(val);
		updates_++;
	}


        bool parse(std::string const& s) {

          if (!s.size()) return false;
          std::vector<std::string> parts;
          boost::split(parts, s, boost::is_any_of("[,]"));
          for (size_t i=0;i<parts.size();i++) {
            if (parts[i].size())
            {
              try {
                vec_.push_back( boost::lexical_cast<T>(parts[i]));
                updates_=1;
              } catch (...) {return false;}
              expectedUpdates_++;
              if (expectedUpdates_>=maxUpdates_) break;
            }
          }
          return true;
        }

        bool append(T const& t)
        {
          if (vec_.size()>=maxUpdates_) return false;
          vec_.push_back(t);
          if (expectedUpdates_<vec_.size()) expectedUpdates_=vec_.size();
          if (!updates_) updates_=1;
          return true;
        }


protected:
	std::vector<T> vec_;
	size_t expectedUpdates_=0;
	size_t maxUpdates_ = JSON_DEFAULT_MAXUPDATES;
};


//fixed size histogram
template<class T,class T2> class HistoJ: public JsonMonitorable {

public:
	HistoJ( int length) : JsonMonitorable(), histo_(length), len_(length) {
	        resetValue();
	}
	HistoJ( std::vector<T>*histo) : JsonMonitorable() {
                histo_ = *histo;
                len_ = histo_.size();
	}
	virtual ~HistoJ() {
        }

        virtual JsonMonitorable* clone();

        virtual JsonMonitorable* cloneType() {
          return new HistoJ<T,T2>(len_);
        }

        virtual JsonMonitorable* cloneAggregationType(OperationType op, SnapshotMode sm, size_t bins=0, size_t expectedUpdates=0, size_t maxUpdates=0) {
          return cloneType();
        }

	virtual std::string toString() const {
		std::stringstream ss;
		ss << "[";
		if (histo_.size())
		  for (unsigned int i = 0; i < histo_.size(); i++) {
		    ss << histo_[i];
		    if (i<histo_.size()-1) ss << ",";
		  }
		ss << "]";
		return ss.str();
	}

	virtual Json::Value toJsonValue() const {
                Json::Value val(Json::arrayValue);
		for (auto& h: histo_) val.append(Json::Value(h));
                return val;
	}

	virtual void resetValue() {
                for (unsigned int i=0;i<len_;i++) histo_[i]=0;
	}

        virtual bool update(JsonMonitorable* mon,SnapshotMode sm, OperationType op)
        {
                if (sm==SM_TIMER) {
                  if (op==OP_HISTO) return updateWithScalar(static_cast<T2*>(mon));
                  else assert(0);
                }
                else return updateWithHisto(static_cast<HistoJ<T,T2>*>(mon));
        return true;
        }

        //todo:update or sum?
        bool updateWithScalar(T2* t) {
          return false;//TODO..

          if (!updates_) {
            if (t->getUpdates())
              histo_[t->value()]=1;
          }
          else if  (t->getUpdates())
            histo_[t->value()]+=1;
          updates_+=t->getUpdates();
          return true;
        }

        bool updateWithHisto(HistoJ<T,T2>* t) {
          if (histo_.size()!=t->getSize()) return false;
          if (!updates_) {
            if (t->getUpdates())
              for (size_t i=0;i<t->value().size();i++)
                histo_[i]=t->value()[i];
          }
          else if (t->getUpdates()) {
            for (size_t i=0;i<t->value().size();i++)
              histo_[i]+=t->value()[i];
          }
          updates_+=t->getUpdates();
          return true;
        }

	std::vector<T> & value() {
		return histo_;
	}

	unsigned int getSize() {
		return histo_.size();
	}

	void update(T val, size_t index) {
		if (index>=len_) return;
		histo_[index]+=val;
	}

	void set(T val, size_t index) {
		if (index>=len_) return;
		histo_[index]=val;
	}


        bool parse(std::string const& s) {

          if (!s.size()) return false;
          std::vector<std::string> parts;
          boost::split(parts, s, boost::is_any_of("[,]"));
          size_t count =0;
          for (size_t i=0;i<parts.size();i++) {
            if (parts[i].size())
            {
              try {
                histo_.push_back( boost::lexical_cast<T>(parts[i]));
                len_++;
                updates_=1;
              } catch (...) {return false;}
              count++;
            }
          }
          return true;
        }

        void append(T const& t)
        {
          histo_.push_back(t);
          len_++;
          if (!updates_) updates_=1;
        }

private:
	std::vector<T> histo_;
        size_t len_=0;
};

//template class HistoJ<double>;
template class VectorJ<long,IntJ>;
template class VectorJ<double,DoubleJ>;
template class VectorJ<std::string,StringJ>;
template class HistoJ<long,IntJ>;
//template class HistoJ<unsigned int,IntJ>;


}

#endif
