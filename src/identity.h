
#ifndef _id_h_
#define _id_h_

#include <stdint.h>

/*
 * @class namespace_id
 *
 * Type-based identity generator/assigner used by task code to handle integer
 * IDs needed by the graphing library.
 */
template<typename C, typename T = uint64_t>
class namespace_id {
public:
    T next_id()
    {
        return _next_id_val++;
    }
private:
    static T _next_id_val;
};

template<typename C, typename T>
T namespace_id<C,T>::_next_id_val = static_cast<T>(0);

#endif // _id_h_
