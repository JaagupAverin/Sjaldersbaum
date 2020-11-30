#pragma once

#include <algorithm>
#include <vector>

/*------------------------------------------------------------------------------------------------*/

template <typename T, typename AssociativeContainer>
inline bool contains(const AssociativeContainer& container, const T& key)
{
    return (container.find(key) != container.cend());
};

template<typename T>
inline bool contains(const std::vector<T>& vector, const T& t)
{
    return std::find(vector.cbegin(), vector.cend(), t) != vector.cend();
}