#include <iostream>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>
#include "ArrayData.h"


ArrayData*  ArrayData::set_array(std::string name, const float *values, int count)
{
    
    std::vector<float> &v = arrays[name];
	v.resize( count );
    std::copy(values, values + count, v.begin());
    return this;

}

ArrayData*  ArrayData::set_array(std::string name, const std::vector<float> &values)
{
    std::vector<float>& v = arrays[name];
    v.resize( values.size() );
    std::copy( values.begin(), values.end(), v.begin( ) );

	return this;
}

ArrayData*  ArrayData::set_array(std::string name, float single_value, int size)
{
	arrays[name].resize(size, single_value);
	return this;
}

//gets the array named name. It such array does not exist, a new empty one is created. 
std::vector<float>& ArrayData::get_array(std::string name)
{
	return arrays[name];
}

std::vector<float>& ArrayData::get_or_create_array(std::string name, float default_value , int size )
{
	if (!contains(name))
	{
		set_array(name, default_value, size );
	}
	return arrays[name];
}

ArrayData& ArrayData::operator=(const ArrayData &idata)
{
    if (&idata != this)
    {
        arrays.clear();

        for (c_iterator it = idata.arrays.begin(); it != idata.arrays.end(); ++it)
        {
            set_array(it->first, it->second);
        }
    }
    return *this;
}


 