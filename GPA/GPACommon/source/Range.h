#ifndef RANGE_H_
#define RANGE_H_ 1
#ifdef GPACOMMON_EXPORTS
#define GPACOMMON_API __declspec(dllexport)
#else
#define GPACOMMON_API __declspec(dllimport)
#endif

#include <iterator>

/*
This is a generator. Its simple purpose is to be able to write shorer loops. Example:

for( auto i : IntRange(0,100)) //i will always be a const
 {
   // i = 0,1,2,....
   /do something.
 }

*/
class
#ifdef ISDLL
    GPACOMMON_API
#endif
    IntRange
{
    /*
    This is just a helper class to implement the range generator.
    This is not intended to be used directly.
    */
    class
#ifdef ISDLL
        GPACOMMON_API
#endif
        num_iterator
    {
    public:

        using iterator_category = std::input_iterator_tag;
        using value_type = int;
        using difference_type = int;
        using pointer = int *;
        using reference = int&;

        explicit num_iterator(const int initial_value = 0) : current(initial_value) { };

        num_iterator(const  num_iterator &it)  noexcept
        {
            current = it.current;
        }

        pointer operator->() noexcept { return &current; }

        reference operator*() noexcept { return current; }

        num_iterator& operator++()noexcept
        {
            current++;
            return *this;
        }

        //post increment
        num_iterator operator++(int)
        {
            num_iterator tmp(*this);
            current++;
            return tmp;
        }

        bool operator==(const num_iterator &it) const noexcept
        {
            return current == it.current;
        }

        bool operator!=(const num_iterator &it) const noexcept
        {
            return it.current != current;
        }

    private:

        int current;
    };
    
public:

    IntRange(int min_value = 0, int max_value = 3) : _start{ min_value }, _end{ max_value } { }

    num_iterator begin() { return _start; }

    num_iterator end() { return _end; }

private:

    num_iterator _start, _end;
};

#endif
