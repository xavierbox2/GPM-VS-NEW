#include "utils.h"

bool finder( std::string s, std::initializer_list<std::string> l )
{
    for(auto it = l.begin( ); it != l.end( ); it++) if(s.find( *it ) != s.npos) return true;
    return false;
}

 
bool exact_match ( std::string s, const vector<string> &l )
{
    for(auto it = l.cbegin( ); it != l.cend( ); it++) if(s == *it) return true;
    return false;
}