#ifndef GPM_VS_INIT_BASE_H_
#define GPM_VS_INIT_BASE_H_ 1

#include <iostream>
#include <string>
#include <vector>

using namespace std;
class IConfiguration
{
public:


    virtual void initialize_vs_options( ) = 0;

    virtual void initialize_keywords( ) = 0;

    virtual ~IConfiguration( ) {}

    virtual void talk( ) = 0;

    virtual vector<string> OutputArraysNames( ) { return vector<string>( ); }
};

#endif
