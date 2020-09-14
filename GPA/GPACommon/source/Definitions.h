
#ifndef DEFINITIONS_H_   
#define DEFINITIONS_H_ 1

#include <iostream>
#include <string>
#include <vector>
#include <set>


#ifdef GPACOMMON_EXPORTS
#define GPACOMMON_API __declspec(dllexport)
#else
#define GPACOMMON_API __declspec(dllimport)
#endif

using namespace std;


//template<typename T>
class
#ifdef ISDLL
    GPACOMMON_API
#endif
    WellKnownVisageNames
{
public:

    class
#ifdef ISDLL
        GPACOMMON_API
#endif


    ResultsArrayNames
    {

    public:

        static set<string> EffectiveStressTensor( )
        {
            return { "EFFSTRXX","EFFSTRYY", "EFFSTRZZ","EFFSTRXY", "EFFSTRYZ", "EFFSTRZX" };
        }

        static set<string> StressTensor( )
        {
            return { "TOTSTRXX","TOTSTRYY", "TOTSTRZZ","TOTSTRXY", "TOTSTRYZ", "TOTSTRZX" };
        }

        static set<string> StrainTensor( )
        {
            return { "STRAINXX","STRAINYY", "STRAINZZ","STRAINXY", "STRAINYZ", "STRAINZX" };
        }

        static set<string> PlasticStrainTensor( )
        {
            return { "PLSTRNXX","PLSTRNYY", "PLSTRNZZ","PLSTRNXY", "PLSTRNYZ", "PLSTRNZX" };
        }

 
        static set<string> YieldIndicators( )
        {
            return { "YIELDMOD","YLDVAL_F", "YLDVAL_I" };
        }

        static set<string> ElementalDisplacement( )
        {
            return { "ROCKDISX","ROCKDISY","ROCKDISZ","NRCKDISZ" };
        }
 

        static string Stiffness;// = "YOUNGMOD" ;
        static string Pressure;
        static string Porosity;

        static set<string> All( )
        {
            set<string> all = { Stiffness, Pressure, Porosity };
            
            set<string> aux = ElementalDisplacement( );
            all.insert( aux.begin(), aux.end() );

            aux = YieldIndicators( );
            all.insert( aux.begin( ), aux.end( ) );

            aux = PlasticStrainTensor( );
            all.insert( aux.begin( ), aux.end( ) );

            aux = StrainTensor();
            all.insert( aux.begin( ), aux.end( ) );

            aux = StressTensor( );
            all.insert( aux.begin( ), aux.end( ) );

            aux = EffectiveStressTensor( );
            all.insert( aux.begin( ), aux.end( ) );

         

            

            return all;
        }

    };


    class
#ifdef ISDLL
        GPACOMMON_API
#endif


   VisageInputKeywords
    {

    public:

        static set<string> SolverKeywords( )
        {      return  { "Nquickcalculation", "nyield_gp_number", "Nsub_increments", "Niterations"};
        }   


    };


};



#endif 

