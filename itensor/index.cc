//
// Distributed under the ITensor Library License, Version 1.2
//    (See accompanying LICENSE file.)
//
#include "itensor/index.h"
#include "itensor/util/readwrite.h"

namespace itensor {

using std::string;
using std::stringstream;


string 
putprimes(string s, int plev)
    { 
    stringstream str;
    str << s;
    if(plev < 0) Error("Negative prime level");
    if(plev > 3)
        {
        str << "'" << plev;
        }
    else
        {
        for(int i = 1; i <= plev; ++i) str << "\'";
        }
    return str.str();
    }

string 
nameindex(const IndexType& it, int plev)
    { 
    return putprimes(it.c_str(),plev); 
    }

string 
nameint(string const& f, int n)
    { 
    return format("%s%d",f,n);
    }

//
// class Index
//



Index::id_type Index::
generateID()
    {
    static Index::IDGenerator G;
    return G();
    }

Index::
Index() 
    : 
    id_(0),
    primelevel_(0),
    m_(1),
    type_(NullInd)
    { }

Index::
Index(const std::string& name, long m, IndexType type, int plev) 
    : 
    id_(generateID()),
    primelevel_(plev),
    m_(m),
    type_(type),
    name_(name.c_str())
    { 
#ifdef DEBUG
    if(type_ == All) Error("Constructing Index with type All disallowed");
    if(type_ == NullInd) Error("Constructing Index with type NullInd disallowed");
#endif
    std::string strname;
    int strplev;
    bool wildcard;
    int strplevincrease;
    splitRawnamePrimelevel(name, strname, strplev, wildcard, strplevincrease);
    if(wildcard) Error("No * in name when constructing Index");
    if(strplev != 0)
        {
        primelevel_ = strplev;
        name_ = IndexName(strname.c_str());
        }
    }

Index& Index::
primeLevel(int plev) 
    { 
    primelevel_ = plev; 
#ifdef DEBUG
    if(primelevel_ < 0)
        Error("Negative primeLevel");
#endif
    return *this;
    }


Index& Index::
prime(int inc) 
    { 
    primelevel_ += inc; 
#ifdef DEBUG
    if(primelevel_ < 0)
        {
        Error("Negative primeLevel");
        }
#endif
    return *this;
    }


string Index::
name() const  { return putprimes(name_.c_str(),primelevel_); }

Index& Index::
rename(std::string const& str) 
    {
    std::string strname;
    int strprimelevel;
    bool wildcard;
    int primeincrease;
    splitRawnamePrimelevel(str, strname, strprimelevel, wildcard, primeincrease);
    name_ = IndexName(strname.c_str());
    if(not wildcard) 
        {
        primelevel_ = strprimelevel;
        }
    else
        {
        if(strprimelevel > 0) Error("No primes before *");
        primelevel_ += primeincrease;
        }
    return *this;
    }

Index
rename(Index i, std::string const& s) 
    { 
    return i.rename(s);
    }

Index Index::
operator()(std::string const& s)
    {
    auto i = *this;
    return i.rename(s);
    }

Index::
operator bool() const { return (id_!=0); }


Index& Index::
mapprime(int plevold, int plevnew, IndexType type)
    {
    if(primelevel_ == plevold)
        {
        if(type == All || type == this->type())
            {
            primelevel_ = plevnew;
#ifdef DEBUG
            if(primelevel_ < 0)
                {
                Error("Negative primeLevel");
                }
#endif
            }
        }
    return *this;
    }


Index& Index::
prime(IndexType type, int inc)
    {
    if(type == this->type() || type == All)
        {
        primelevel_ += inc;
#ifdef DEBUG
        if(primelevel_ < 0)
            {
            Error("Increment led to negative primeLevel");
            }
#endif
        }
    return *this;
    }

Index& Index::
prime(std::string const& str, int inc)
    {
    if(nameMatch(*this,str))
        {
        primelevel_ += inc;
#ifdef DEBUG
        if(primelevel_ < 0)
            {
            Error("Increment led to negative primeLevel");
            }
#endif
        }
    return *this;
    }


bool Index::
noprimeEquals(Index const& other) const
    { 
    return (id_ == other.id_);
    }

IndexVal Index::
operator()(long val) const
    {
    return IndexVal(*this,val);
    }

Index Index::
operator[](int plev) const
    { 
    auto I = *this;
    I.primeLevel(plev); 
    return I; 
    }

void Index::
write(std::ostream& s) const 
    { 
    if(!bool(*this)) Error("Index::write: Index is default initialized");
    itensor::write(s,primelevel_);
    itensor::write(s,type_);
    itensor::write(s,id_);
    itensor::write(s,m_);
    itensor::write(s,name_);
    }

Index& Index::
read(std::istream& s)
    {
    itensor::read(s,primelevel_);
    itensor::read(s,type_);
    if(Global::read32BitIDs())
        {
        using ID32 = std::mt19937::result_type;
        ID32 oldid = 0;
        itensor::read(s,oldid);
        id_ = oldid;
        }
    else
        {
        itensor::read(s,id_);
        }
    itensor::read(s,m_);
    itensor::read(s,name_);

#ifdef DEBUG
    if(primelevel_ < 0) Error("Negative primeLevel");
#endif

    return *this;
    }

void
analyzePrimeString(std::string const& str, int & primelevel)
    {
    auto N = str.length();
    if(str.substr(0,1) != "'")
        {
        Error("Prime string must start with '");
        }
    else
        {
        if(N == 1)
            {
            primelevel = 1;
            }
        else
            {
            std::string matchstr(N-1, '\'');
            auto endstr = str.substr(1,N);
            if(matchstr == endstr)
                {
                primelevel = N;
                }
            else
                {
                primelevel = std::stoi(endstr);
                }
            }
        }
    }

void
splitRawnamePrimelevel(std::string const& startstr, std::string & rawname, 
                       int & primelevel, bool & wildcard, int & primeincrease)
    {
    // Check location of "*"
    auto w = startstr.find("*");
    auto N = startstr.length();
    auto str = startstr;
    if(w == std::string::npos)
        {
        wildcard = false;
        primeincrease = 0;
        }
    else if(w == N-1)
        {
        wildcard = true;
        str = startstr.substr(0,w);
        primeincrease = 0;
        }
    else
        {
        wildcard = true;
        str = startstr.substr(0,w);
        analyzePrimeString(startstr.substr(w+1,N), primeincrease);
        }

    N = str.length();
    auto i = str.find("'");
    if(i == std::string::npos)
        {
        rawname = str.c_str();
        primelevel = 0;
        }
    else
        {
        rawname = str.substr(0,i).c_str();
        analyzePrimeString(str.substr(i,N), primelevel);
        }
    }

bool
nameMatch(Index const& ind, std::string const& str)
    {
    auto rawname1 = ind.rawname();
    auto primelevel1 = ind.primeLevel();

    std::string rawname2;
    int primelevel2;
    bool wildcard2;
    int primeincrease2;
    splitRawnamePrimelevel(str, rawname2, primelevel2, wildcard2, primeincrease2);

    if(wildcard2)
        {
        return (rawname1 == rawname2) && (primelevel1 >= primelevel2);
        }
    else
        {
        return (rawname1 == rawname2) && (primelevel1 == primelevel2);
        }
    }

bool 
operator==(Index const& i1, Index const& i2)
    { 
    return (i1.id() == i2.id()) && (i1.primeLevel() == i2.primeLevel()) && (i1.rawname() == i2.rawname()); 
    }

bool 
operator!=(Index const& i1, Index const& i2)
    { 
    return not operator==(i1,i2);
    }

bool
operator>(Index const& i1, Index const& i2)
    { 
    if(i1.m() == i2.m()) 
        {
        if(i1.id() == i2.id()) return i1.primeLevel() > i2.primeLevel();
        return i1.id() > i2.id();
        }
    return i1.m() > i2.m();
    }

bool
operator<(Index const& i1, Index const& i2)
    {
    if(i1.m() == i2.m()) 
        {
        if(i1.id() == i2.id()) return i1.primeLevel() < i2.primeLevel();
        return i1.id() < i2.id();
        }
    return i1.m() < i2.m();
    }




std::ostream& 
operator<<(std::ostream & s, Index const& t)
    {
    s << "(\"" << t.rawname();
    s << "\"," << t.m();
    s << "," << t.type().c_str();
    if(Global::showIDs()) 
        {
        s << "|" << (t.id() % 1000);
        //s << "," << t.id();
        }
    s << ")"; 
    if(t.primeLevel() > 0) 
        {
        if(t.primeLevel() > 3)
            {
            s << "'" << t.primeLevel();
            }
        else
            {
            for(int n = 1; n <= t.primeLevel(); ++n)
                s << "'";
            }
        }
    return s;
    }

IndexVal::
IndexVal() 
    : 
    val(0) 
    { }

IndexVal::
IndexVal(const Index& index_, long val_) 
    : 
    index(index_),
    val(val_)
    { 
#ifdef DEBUG
    if(!index) Error("IndexVal initialized with default initialized Index");
    //Can also use IndexVal's to indicate prime increments:
    //if(val_ < 1 || val_ > index.m())
    //    {
    //    println("val = ",val_);
    //    println("index = ",index);
    //    Error("val out of range");
    //    }
#endif
    }

bool
operator==(IndexVal const& iv1, IndexVal const& iv2)
    {
    return (iv1.index == iv2.index && iv1.val == iv2.val);
    }

bool
operator!=(IndexVal const& iv1, IndexVal const& iv2)
    {
    return not operator==(iv1,iv2);
    }

bool
operator==(Index const& I, IndexVal const& iv)
    {
    return iv.index == I;
    }

bool
operator==(IndexVal const& iv, Index const& I)
    {
    return iv.index == I;
    }

Index
sim(Index const& I, int plev)
    {
    return Index("~"+I.rawname(),I.m(),I.type(),plev);
    }

string
showm(Index const& I) { return nameint("m=",I.m()); }


std::ostream& 
operator<<(std::ostream& s, IndexVal const& iv)
    { 
    return s << "IndexVal: val = " << iv.val 
             << ", ind = " << iv.index;
    }

void
add(Args            & args, 
    Args::Name const& name, 
    IndexType         it) 
    { 
    args.add(name,it.c_str()); 
    }

IndexType
getIndexType(Args       const& args, 
             Args::Name const& name)
    {
    if(!args.defined(name)) Error(format("Name %s not found in Args",name));
    return IndexType(args.getString(name).c_str());
    }

IndexType
getIndexType(const Args& args, 
             const Args::Name& name, 
             IndexType default_val)
    {
    if(!args.defined(name)) return default_val; 
    return IndexType(args.getString(name).c_str());
    }

} //namespace itensor

