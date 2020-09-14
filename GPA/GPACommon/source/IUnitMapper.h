#ifndef IUNITS_H_
#define IUNITS_H_ 1

#include <iostream>
#include <string>
#include <unordered_map>
 

using namespace std;

class IUnitMapper
{

public:

    virtual ~IUnitMapper() { ; }

    virtual size_t size() const noexcept = 0;

    virtual void set_property(const string &s, float gain, float offset, string unit_symbol = "-") = 0;

    virtual bool knows_property(const string &s) const noexcept = 0;

    virtual const tuple<float, float, string>& operator[](const string& name) const = 0;

};

#endif 

