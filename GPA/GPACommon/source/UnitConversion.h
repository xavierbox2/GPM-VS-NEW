//#ifndef UNITS_MAPPER_H_
//#define UNITS_MAPPER_H_ 1
//
//#include <iostream>
//#include <string>
//#include <unordered_map>
//#include "IUnitMapper.h"
//
//using namespace std;
//
//class UnitMapper : public IUnitMapper
//{
//public:
//
//    UnitMapper() = default;
//
//    UnitMapper(const UnitMapper &m) = default;
//
//    UnitMapper(UnitMapper &&m) = default;
//
//    virtual ~UnitMapper() override { ; }
//
//    UnitMapper* operator->() noexcept { return this; }
//
//    UnitMapper& operator = (UnitMapper &&m) = default;
//
//    UnitMapper& operator = (const UnitMapper &m) = default;
//
//    virtual size_t size() const noexcept override { return _unit_map.size(); }
//
//    virtual void set_property(const std::string &s, float gain, float offset, std::string unit_symbol = "-") override
//    {
//        _unit_map[s] = make_tuple(gain, offset, unit_symbol);
//    }
//
//    virtual bool knows_property(const std::string &s) const  noexcept override
//    {
//        return !(_unit_map.cend() == _unit_map.find(s));
//    }
//
//    virtual const tuple<float, float, std::string>&  operator[](const std::string& name) const override
//    {
//        return  _unit_map.at(name);
//    }
//
//    const tuple<float, float, std::string>&  at(const std::string& name) const
//    {
//        return  _unit_map.at(name);
//    }
//
//    bool contains(const std::string &s) const noexcept
//    {
//        return knows_property(s);
//    }
//
//protected:
//
//    unordered_map< std::string, tuple<float, float, std::string> > _unit_map; //property name, gain, offset unit name
//};
//
//class PetrelVisageResultsUnits : public UnitMapper
//{
//public:
//
//    PetrelVisageResultsUnits() : UnitMapper()
//    {
//        //set_property("YOUNGSMOD");
//    }
//};
//
//class VisageInputFilesUnits : public UnitMapper
//{
//public:
//
//    VisageInputFilesUnits() : UnitMapper()
//    {
//        //set_property("YOUNGSMOD");
//    }
//};
//
//class GPMDisplayUnits : public UnitMapper
//{
//public:
//
//    GPMDisplayUnits() : UnitMapper()
//    {
//        //set_property("YOUNGSMOD");
//    }
//};
//
//#endif
