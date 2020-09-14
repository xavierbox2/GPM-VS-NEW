#ifndef BASE_TYPES_SIM_OPTIONS_H
#define BASE_TYPES_SIM_OPTIONS_H 1

#include <string>
#include <vector>
#include <iostream>
#include <map>
#include <unordered_map>
#include <algorithm>
#include <cctype>

#ifdef VISAGEDECKWRITEC_EXPORTS
#define VISAGEDECKWRITEC_API __declspec(dllexport)
#else
#define VISAGEDECKWRITEC_API __declspec(dllimport)
#endif

using namespace std;

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    Instruction
{
public:

    Instruction( Instruction& k ) = default;

    Instruction( std::string preffix = "*", string value = "", string name = "" )
        : preffix( preffix ), value( value ), name( name ) {
        ;
    }

    string preffix, value, name;

    string to_string( )
    {
        if(name.empty( )) return "";

        string s = preffix + name + "\n";
        if(!value.empty( ))            //the values printed below the *xxxx
        {
            s += value;
            s += "\n";
        }
        return s;
    }
};

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
InstructionBlock: Instruction
{
public:

    //copy constructo;
    InstructionBlock( InstructionBlock & k )
    {
        Name = k.Name; Value = k.Value; Preffix = k.Preffix;
        replace_hashes( &k.hashedValues );
    }

    //default constructor
    InstructionBlock( string name = "", string value = "", string preffix = "*", unordered_map<string, string> * hashes = nullptr ) :Instruction( name, value, preffix )
    {
        Value = value; Preffix = preffix; Name = name;
        replace_hashes( hashes );
    }

    void set_replace_instruction( string name, string value )
    {
        hashedValues[name] = value;
    }

    void set_instruction( unordered_map<string, string> * hashes )
    {
        if((hashes == nullptr) || (hashes->empty( ))) return;
        for(auto it = hashes->begin( ); it != hashes->end( ); ++it)
            set_replace_instruction( it->first, it->second );
    }


    void delete_instruction( string name )
    {
        unordered_map<string, string>::iterator it = hashedValues.find( name );
        if(it != hashedValues.end( ))
            hashedValues.erase( it );
    }

    //the mii writter depends on this
    string to_string( ) const
    {
        if(Name.empty( )) return "";

        string s = Preffix + Name + "\n";
        if(!(Value.empty( )))            //the values printed below the *xxxx
        {
            //if the value is a comma separated string, then each is a value
            s += Value;
            s += "\n";
        }

        if(!hashedValues.empty( )) {
            for(auto it = hashedValues.cbegin( ); it != hashedValues.cend( ); ++it)
            {
                s += ("#" + (*it).first + "\t" + (*it).second + "\n");
            }
            s += "#end\n";
        }
        return s;
    }

    bool has_hash( string  s ) const //, bool ignorecase = true ) const
    {   return hashedValues.find( s ) == hashedValues.end( ) ? false : true;
    }

    std::unordered_map<string, string> hashedValues;
    string Name, Value, Preffix;

private:



    void replace_hashes( unordered_map<string, string>* hashes )
    {
        if(!(hashedValues.empty( )))
            hashedValues.clear( );

        if((hashes == nullptr) || (hashes->empty( )))
            return;

        for(auto it = hashes->begin( ); it != hashes->end( ); ++it)
            hashedValues.insert( std::pair<string, string>( (*it).first, (*it).second ) );
    }


};

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    SimulationOptions
{
public:

    SimulationOptions( string name, int step ) : _model_name( name ), _step( step ) { ; }

    SimulationOptions* operator->( ) { return this; }

    virtual ~SimulationOptions( )
    {
        delete_commands( );
    }

    virtual string to_string( ) = 0;

    virtual string to_string( int time_step ) = 0;

    virtual string model_name( ) const { return _model_name; }

    virtual string& model_name( ) { return _model_name; }

    virtual std::string path( ) const { return _path; }

    virtual std::string& path( ) { return _path; }

    virtual bool set_value( string name, string value );

    virtual bool contains( string  s ) const;

    virtual InstructionBlock* set_command( string name, string value = "", unordered_map<string, string>* hashes = nullptr );

    virtual InstructionBlock* get_command( std::string name );

    virtual std::vector< InstructionBlock* >  get_commands( ) const;

    template<typename T>
    void set_instruction( string commandName, string hashName, T hashValue )
    {
        remove_space( commandName );
        remove_space( hashName );
        set_replace_instruction( commandName, hashName, std::to_string( hashValue ) );
    }

    //hashes are options inside the asterisks/commands. Both can also have values. They look as hashes and asterisks in the miis
    InstructionBlock* set_replace_instruction( string commandName, string hashName, string hashValue );

    virtual void delete_command( string name );

    virtual void delete_instruction( string command, string name );

    virtual void use_options( unordered_map<string, string> options );

    virtual int& step( ) { return _step; }

    virtual int step( ) const { return _step; }

protected:

    int _step;
    std::string _model_name, _path;
    unordered_map<string, InstructionBlock*> commands;

    void delete_commands( )
    {
        for(unordered_map<string, InstructionBlock*>::iterator it = commands.begin( ); it != commands.end( ); ++it)
            delete it->second;

        commands.clear( );
    }

    static void remove_space( std::string& s )
    {
        s.erase( std::remove_if( s.begin( ), s.end( ), ::isspace ), s.end( ) );
    }
};

#endif
