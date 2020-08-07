#ifndef GPM_VS_INIT_BASE_H_
#define GPM_VS_INIT_BASE_H_ 1 

#include <iostream>

class gpm_vs_config
{
 public: 

  virtual void initialize_vs_options() = 0; 

  virtual void initialize_keywords() = 0; 
  
  virtual ~gpm_vs_config()
  {
  }

  virtual void talk() = 0; 


};

class default_elastic_config: public gpm_vs_config
{
public:

    default_elastic_config()
    {
    
    }

    virtual void initialize_vs_options( ) override
    {;
    }

    virtual void initialize_keywords( ) override 
    {;
    }

    virtual void talk( ) override 
    {
     std::cout<<"I am  the default elastic configurator. I was  injected during construction "<<std::endl; 
    }

};


#endif 
