#ifndef SED_DESC_H_
#define SED_DESC_H_ 1

#include <string>
#include <unordered_map>
#include <map>
#include <vector>
#include <iterator>
#include <algorithm>

#include "vs_api/ArrayData.h"
#include "vs_api/Range.h"

using namespace std;

class sediment_description
{
public:

    sediment_description( )
    {
        atts["YOUNGMOD"] = 10.0e10f;
        atts["POISSONR"] = 0.3f;
        atts["Id"] = 1;
        atts["DENSITY"] = 2700.00f;
        atts["POROSITY"] = 0.5f;
        atts["Diameter"] = 1.0f;
        name = "Sand(coarse)";
    }

    bool contains( std::string name ) const
    {
        return atts.find( name ) != atts.end( );
    }

    float& operator( )( string att_name )
    {
        return atts[att_name];
    }

    float& operator[]( string att_name )
    {
        return atts[att_name];
    }

    void print( )
    {
        for(auto &p : atts) cout << "  " << p.first << "  " << p.second << endl;
    }

    string name;
    unordered_map<string, float> atts;

    static map<string, sediment_description> get_default_sediments( int num = 4 )
    {
        map<string, sediment_description> seds;
        for(int n : IntRange( 0, num ))
        {
            string name = "SED" + to_string( n + 1 );
            seds[name] = sediment_description( );
            seds[name]["YOUNGMOD"] = 1.0e7f;//  3.0e5- (n + 1)*0.5e5;
            seds[name]["POROSITY"] = 0.30f;//5 + 0.05*(n + 1);
            seds[name]["DENSITY"] = 2700.0f;// + 50.0*(n + 1);
            seds[name]["Diameter"] = 0.1f;//*(n + 1);
            seds[name]["Id"] = (float)(n + 1);
        }

        return seds;
    }
};

#endif

/*
"SEDIMENTS": [
{
    "Name": "Copy of Sand (coarse)",
        "Id" : "://SedimentDataSource/a5c6774d-b2ed-4079-8249-a46fc883eeb8",
        "Diameter" : 1.0,
        "Density" : 2.7,
        "InitialPorosity" : 0.3,
        "InitialPermeability" : 100.0,
        "CompactedPorosity" : 0.2,
        "CompactedPermeability" : 10.0,
        "Compaction" : 5000.0,
        "PermeabilityAnisotropy" : 1.0,
        "Transportability" : 0.0,
        "ErodabilityCoefficient" : 1.0,
        "InplaceTransformation" : 0,
        "InplaceTransformationCoefficient" : 0.0
},
*/