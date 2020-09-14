#ifndef DATA_SOURCE_H  
#define DATA_SOURCE_H 1

#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <functional>

using namespace std;

class
#ifdef ISDLL
    GPACOMMON_API
#endif
    IData
{
public:

    virtual ~IData() = default;

    virtual size_t size() const noexcept = 0;

};


class
#ifdef ISDLL
    GPACOMMON_API
#endif
    IDataSource
{
public:

    virtual IData*  get_values() = 0;

    virtual void set_values(const IData *tabular) = 0;

    virtual ~IDataSource() = default;

};


#endif 
