#ifndef UI_PARAMETERS_H_ 
#define UI_PARAMETERS_H_ 1

#include <numeric>
#include <algorithm>
#include <iterator>
#include <vector>
#include <set>
#include <iostream>
#include <string>

#include "Table.h"

using namespace std;

struct SedimentDescription
{
public:

    SedimentDescription( ) = default;

    set<string> property_names( ) const {
        set<string> keys;
        transform( properties.begin( ), properties.end( ), inserter( keys, keys.begin( ) ),
                   []( const auto& p )
                   { return p.first;
                   } );
        return keys;
    }

    Table compaction_table;
    map<string, float> properties;
    string id;
    int index;

};

struct UIParameters
{
public:

    friend ostream& operator<<( ostream& out, const UIParameters& ui_params )
    {
        stringstream stream;
        stream << boolalpha;

        if(ui_params.sediments.size( ) > 0)
        {
            out << "Sediment properties:" << endl;


            for(const auto& pair : ui_params.sediments)
            {
                const auto& s = pair.second;
                for(const auto& name : s.property_names( ))
                    out << setw( 10 ) << name << " "; cout << endl;

                //for(const auto& sediment : ui_params.sediments)
                //{
                for(const auto& ppair : s.properties)
                    out << setw( 10 ) << ppair.second << " "; cout << endl;
            //}
            }
        }

        for(const auto flag : ui_params.flags)
            out << flag.first << ":" << flag.second << endl;

        for(const auto prop : ui_params.properties)
            out << prop.first << ":" << prop.second << endl;

        for(const auto name : ui_params.names)
            out << name.first << ":" << name.second << endl;

        return out;
    }

    UIParameters( ) = default;

    map<string, bool> flags;
    map<string, float> properties;
    map<string, string> names;
    map<string, SedimentDescription> sediments;

    Table plasticity_multiplier, strain_function; 


};

#endif 
