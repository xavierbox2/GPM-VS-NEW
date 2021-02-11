
#ifdef VISAGEDECKWRITEC_EXPORTS
#define VISAGEDECKWRITEC_API __declspec(dllexport)
#else
#define VISAGEDECKWRITEC_API __declspec(dllimport)
#endif
#include <algorithm>
#include <numeric>
#include <iterator>

#include "BaseTypesSimulationOptions.h"



bool SimulationOptions::set_value( std::string name, std::string value )
{
    remove_space( name );
    bool ret = false;
    //does name start with * -> it is a command
    if(name[0] == '*')
    {
        name.erase( 0, 1 );
        set_command( name, value );
        ret = true;
    }

    else if(name[0] == '#') //it is a hash! do we have the command ?
    {
        name.erase( 0, 1 );
        for(auto it = commands.begin( ); it != commands.end( ); ++it)
        {
            if(it->second->has_hash( name ))
            {
                it->second->set_replace_instruction( name, value );
                ret = true;
                break;
            }
        }
    }
    else  if(contains( name ))
    {
        commands[name]->value = value;
        return true;
    }

    else//we dont know what it is. We will assume it is a hash and search for a command that contains it.
    {
        for(auto it = commands.begin( ); it != commands.end( ); ++it)
        {
            if(it->second->has_hash( name ))
            {
                it->second->set_replace_instruction( name, value );
                return true;
            }
        }

    }

    throw ("Cannot set the value " + name + " because the related command is not known");

    return ret;
}

bool SimulationOptions::contains( std::string  s ) const
{
    return commands.find( s ) == commands.end( ) ? false : true;
}

InstructionBlock* SimulationOptions::set_command( std::string name, std::string value, std::unordered_map<std::string, std::string> *hashes )
{
    remove_space( name );
    delete_command( name );

    InstructionBlock *new_command = new InstructionBlock( name, value );
    new_command->set_instruction( hashes );
    commands.insert( std::pair<std::string, InstructionBlock*>( name, new_command ) );
    return new_command;
}



//hashes are options inside the asterisks/commands. Both can also have values. They look as hashes and asterisks in the miis
InstructionBlock* SimulationOptions::set_replace_instruction( std::string commandName, std::string hashName, std::string hashValue )
{
    remove_space( commandName );
    remove_space( hashName );
    InstructionBlock* command = NULL;
    std::unordered_map<std::string, InstructionBlock*>::iterator it = commands.find( commandName );
    if(it != commands.end( ))
    {
        command = it->second;
        command->set_replace_instruction( hashName, hashValue );
    }
    else //create a new command with the hash
    {
        command = set_command( commandName, "" );
        command->set_replace_instruction( hashName, hashValue );
        commands.insert( std::pair<std::string, InstructionBlock*>( commandName, command ) );
    }

    return command;
}


void SimulationOptions::delete_instruction( std::string command, std::string name )
{
    remove_space( name );

    std::unordered_map<std::string, InstructionBlock*>::iterator it = commands.find( command );
    if(it != commands.end( ))
    {
        it->second->delete_instruction( name );
    }
}


void SimulationOptions::delete_command( std::string name )
{
    remove_space( name );

    std::unordered_map<std::string, InstructionBlock*>::iterator it = commands.find( name );
    if(it != commands.end( ))
    {
        delete it->second;
        commands.erase( it );
    }
}

void SimulationOptions::use_options( std::unordered_map<std::string, std::string> options )
{
    bool all_success = true;
    for(auto &option : options)
    {
        all_success &= set_value( option.first, option.second );
    }
}