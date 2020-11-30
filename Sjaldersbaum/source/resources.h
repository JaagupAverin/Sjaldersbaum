#pragma once

#include "rm.h"
#include "string_assist.h"

/*------------------------------------------------------------------------------------------------*/

extern const std::string SYSTEM_FONT_PATH;

/*------------------------------------------------------------------------------------------------*/
// Known types of shared resources:

template<typename T>
class ResourceReference;

using TextureReference     = ResourceReference<sf::Texture>;
using FontReference        = ResourceReference<sf::Font>;
using SoundBufferReference = ResourceReference<sf::SoundBuffer>;

/*------------------------------------------------------------------------------------------------*/

// Refers to a resource of type T. Think shared_ptr, but easier to use.
// Construction/copying increments the underlying resource's reference count.
// Destruction/overcopying decrements the underlying resource's reference count.
// The underlying resource is destructed if its reference count hits 0.
template<typename T>
class ResourceReference
{
public:
    ResourceReference();
    ResourceReference(const std::string& resource_path);
    ResourceReference(const ResourceReference& other);
    ResourceReference(ResourceReference&& other);
    ~ResourceReference();

    ResourceReference& operator=(const ResourceReference& other);
    ResourceReference& operator=(ResourceReference&& other);

    // Loads the resource from specified path and starts referring to it.
    // If a resource has already been loaded from specified path, starts referring to it.
    // If the load is unsuccessful, an empty resource still exists and will be referred to.
    void load(std::string resource_path);

    // Note that the returned resource must only be used for as long as the reference is alive.
    // If no resource is loaded, returns an empty (default-constructed) resource.
    const T& get() const;

    // Returns the path from which the resource was loaded from.
    const std::string& get_path() const;

    // Returns true if the load method has been called. See load(std::string resource_path).
    bool is_loaded() const;

private:
    const T* resource;
    std::string resource_path;
};

/*------------------------------------------------------------------------------------------------*/
// Implementation:

template<typename T>
inline ResourceReference<T>::ResourceReference() :
    resource     { nullptr },
    resource_path{ "" }
{

}

template<typename T>
inline ResourceReference<T>::ResourceReference(const std::string& resource_path) :
    ResourceReference()
{
    load(resource_path);
}

template<typename T>
inline ResourceReference<T>::ResourceReference(const ResourceReference<T>& other) :
    ResourceReference()
{
    *this = other;
}

template<typename T>
inline ResourceReference<T>::ResourceReference(ResourceReference<T>&& other) :
    ResourceReference()
{
    *this = std::move(other);
}

template<typename T>
inline ResourceReference<T>::~ResourceReference()
{
    ResourceManager<T>::instance().decrement_reference_count(resource_path);
}

template<typename T>
inline ResourceReference<T>& ResourceReference<T>::operator=(const ResourceReference<T>& other)
{
    if (this == &other)
        return *this;
    
    if (resource_path != other.resource_path)
    {
        ResourceManager<T>::instance().decrement_reference_count(resource_path);
        ResourceManager<T>::instance().increment_reference_count(other.resource_path);

        resource      = other.resource;
        resource_path = other.resource_path;
    }

    return *this;
}

template<typename T>
inline ResourceReference<T>& ResourceReference<T>::operator=(ResourceReference<T>&& other)
{
    if (this == &other)
        return *this;

    resource      = std::move(other.resource);
    resource_path = std::move(other.resource_path);

    other.resource = nullptr;
    other.resource_path.clear();

    return *this;
}

template<typename T>
inline void ResourceReference<T>::load(std::string resource_path)
{
    decapitalize(resource_path);

    ResourceManager<T>::instance().decrement_reference_count(this->resource_path);
    ResourceManager<T>::instance().increment_reference_count(resource_path);

    resource = &ResourceManager<T>::instance().get(resource_path);
    this->resource_path = std::move(resource_path);
}

template<typename T>
inline const T& ResourceReference<T>::get() const
{
    if (!is_loaded())
    {
        LOG_ALERT("dereferencing an uninitialized reference;\n"
                  "returning a default-constructed resource instead.");
        return ResourceManager<T>::instance().get_default();
    }
    return *resource;
}

template<typename T>
inline const std::string& ResourceReference<T>::get_path() const
{
    return resource_path;
}

template<typename T>
inline bool ResourceReference<T>::is_loaded() const
{
    return resource != nullptr;
}