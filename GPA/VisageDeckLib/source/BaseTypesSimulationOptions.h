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

//using namespace std;

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    Instruction
{
public:

    Instruction( Instruction& k ) = default;

    Instruction( std::string name = "", std::string value = "", std::string preffix = "*" ) :name( name ), value( value ), preffix( preffix )
    {
        ;
    }

    std::string preffix, value, name;

    std::string to_string( )
    {
        if(name.empty( )) return "";

        std::string s = preffix + name + "\n";
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
InstructionBlock: public Instruction
{
public:

    //copy constructo;
    InstructionBlock( InstructionBlock & k )
    {
        name = k.name;
        value = k.value;
        preffix = k.preffix;
        replace_hashes( &k.hashedValues );
    }

    //default constructor
    InstructionBlock( std::string iname = "", std::string value = "", std::string preffix = "*", std::unordered_map<std::string, std::string> * hashes = nullptr ) :Instruction( iname, value, preffix )
    {
       replace_hashes( hashes );
    }

    void set_replace_instruction( std::string name, std::string value )
    {
        hashedValues[name] = value;
    }

 

    void set_instruction( std::unordered_map<std::string, std::string> * hashes )
    {
        if((hashes == nullptr) || (hashes->empty( ))) return;
        for(auto it = hashes->begin( ); it != hashes->end( ); ++it)
            set_replace_instruction( it->first, it->second );
    }


    void delete_instruction( std::string name )
    {
        std::unordered_map<std::string, std::string>::iterator it = hashedValues.find( name );
        if(it != hashedValues.end( ))
            hashedValues.erase( it );
    }

    //the mii writter depends on this
    std::string to_string( ) const
    {
        if(name.empty( )) return "";

        std::string s = preffix + name + "\n";
        if(!(value.empty( )))            //the values printed below the *xxxx
        {
            //if the value is a comma separated std::string, then each is a value
            s += value;
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

    bool has_hash( std::string  s ) const //, bool ignorecase = true ) const
    {
    return hashedValues.find( s ) == hashedValues.end( ) ? false : true;
    }

    std::string get_hash( std::string  s ) const
    {
        return  has_hash( s ) ? hashedValues.at( s ) : "";
    }
    std::string at ( std::string s ) const
    {
    return  has_hash( s ) ? hashedValues.at( s ) : "";
    }
    std::string operator[]( std::string s) const 
    {
    return  has_hash( s ) ? hashedValues.at( s ) : "";
    }
    std::string& operator[]( std::string s ) 
    {
    return hashedValues[ s ] ;
    }



std::unordered_map<std::string, std::string> hashedValues;
//std::string Name, Value, Preffix;

private:



    void replace_hashes( std::unordered_map<std::string, std::string>* hashes )
    {
        if(!(hashedValues.empty( )))
            hashedValues.clear( );

        if((hashes == nullptr) || (hashes->empty( )))
            return;

        for(auto it = hashes->begin( ); it != hashes->end( ); ++it)
            hashedValues.insert( std::pair<std::string, std::string>( (*it).first, (*it).second ) );
    }


};

class
#ifdef ISDLL
    VISAGEDECKWRITEC_API
#endif
    SimulationOptions
{
public:

    SimulationOptions( std::string name, int step ) : _model_name( name ), _step( step ) { ; }

    SimulationOptions* operator->( ) { return this; }

    virtual ~SimulationOptions( )
    {
        delete_commands( );
    }

    virtual std::string to_string( ) = 0;

    virtual std::string to_string( int time_step ) = 0;

    virtual std::string model_name( ) const { return _model_name; }

    virtual std::string& model_name( ) { return _model_name; }

    virtual std::string path( ) const { return _path; }

    virtual std::string& path( ) { return _path; }

    virtual bool set_value( std::string name, std::string value );

    virtual bool contains( std::string  s ) const;

    virtual InstructionBlock* set_command( std::string name, std::string value = "", std::unordered_map<std::string, std::string>* hashes = nullptr );

    virtual InstructionBlock* get_command( std::string name )
    {
        remove_space( name );

        auto it = commands.find( name );
        if(it != commands.end( ))
            return  it->second;

        return nullptr;
    }

    virtual std::string get_hash( std::string command, std::string hash_name )
    {
        if(InstructionBlock* i = get_command( command ); i != nullptr)
        {
            return i->get_hash( hash_name );
        }
        return "";
    }

    virtual std::vector< InstructionBlock* > get_commands( ) const
    {
        std::vector< InstructionBlock* > items;
        //copy( commands.begin( ), commands.end( ), back_inserter( co ) );
        //return co;

        for(std::unordered_map<std::string, InstructionBlock*>::const_iterator it = commands.cbegin( ); it != commands.cend( ); ++it)
        {
            items.push_back( it->second );
        }

        return items;
    }



    template<typename T>
    void set_instruction( std::string commandName, std::string hashName, T hashValue )
    {
        remove_space( commandName );
        remove_space( hashName );
        set_replace_instruction( commandName, hashName, std::to_string( hashValue ) );
    }

    //hashes are options inside the asterisks/commands. Both can also have values. They look as hashes and asterisks in the miis
    InstructionBlock* set_replace_instruction( std::string commandName, std::string hashName, std::string hashValue );

    virtual void delete_command( std::string name );

    virtual void delete_instruction( std::string command, std::string name );

    virtual void use_options( std::unordered_map<std::string, std::string> options );

    virtual int& step( ) { return _step; }

    virtual int step( ) const { return _step; }

protected:

    int _step;
    std::string _model_name, _path;
    std::unordered_map<std::string, InstructionBlock*> commands;

    void delete_commands( )
    {
        for(std::unordered_map<std::string, InstructionBlock*>::iterator it = commands.begin( ); it != commands.end( ); ++it)
            delete it->second;

        commands.clear( );
    }

    static void remove_space( std::string& s )
    {
        s.erase( std::remove_if( s.begin( ), s.end( ), ::isspace ), s.end( ) );
    }
};

#endif
