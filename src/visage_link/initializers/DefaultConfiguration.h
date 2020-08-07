#ifndef GPM_VS_DEFAULT_CONFIG_H_
#define GPM_VS_DEFAULT_CONFIG_H_ 1

#include <iostream>

#include "IConfiguration.h"

class DefaultConfiguration : public IConfiguration
{
public:

    DefaultConfiguration( )
    {
    }

    virtual void initialize_vs_options( ) override
    {
        ;
    }

    virtual void initialize_keywords( ) override
    {
        ;
    }

    virtual void talk( ) override
    {
        std::cout << "I am  the default configurator. I was injected during construction " << std::endl;
    }
};

#endif
