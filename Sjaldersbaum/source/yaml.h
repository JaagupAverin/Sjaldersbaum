#pragma once

#pragma warning(push, 0) // The yaml library has some stuff considered bad style these days.
#include <yaml-cpp/yaml.h>
#pragma warning(pop)

#include <SFML/Graphics.hpp>

#include "logger.h"

/*------------------------------------------------------------------------------------------------*/

namespace YAML
{
class Serializable
{
public:
    virtual bool initialize(const YAML::Node& node) = 0;
    // Dynamic data is data that can change (with respect to the initialization data).
    virtual YAML::Node serialize_dynamic_data() const
    {
        LOG_ALERT("unexpected serialize call; unimplemented method.");
        return YAML::Node{YAML::NodeType::Undefined};
    };
};

// Inserts all values (scalars, sequences) from inserter_node to base_node.
// Note that insertion_path is an implementation detail for recursively calling the function:
// Since only inserter_node has to be traversed, base_node is left unmodified until insertion;
// the path is then used to map the traversed inserter_node into the base_node.
inline void insert_all_values(YAML::Node& base_node, const YAML::Node& inserter_node,
                              const std::vector<std::string>& insertion_path = {})
{
    // Inserts the specified value into base_node using several keys; that is:
    // base_node[key1][key2][...][last_key] = value;
    auto insert = [&base_node](const std::vector<std::string>& keys, const YAML::Node& value)
    {
        if (!value.IsDefined() || value.IsNull())
            return;

        if (keys.empty())
        {
            base_node = value;
            return;
        }

        // Traverse a reference of the base node until we reach the last key,
        // at which we insert the value (replacing any current value).
        YAML::Node subnode = base_node;
        for (size_t i = 0; i != keys.size() - 1; ++i)
            subnode.reset(subnode[keys[i]]);
        subnode[keys.back()] = value;
    };

    // Value found:
    if (!inserter_node.IsMap())
        insert(insertion_path, inserter_node);
    else
    {
        // Recurse through inserter_node maps:
        for (const auto& node : inserter_node)
        {
            auto local_insertion_path = insertion_path;
            local_insertion_path.emplace_back(node.first.Scalar());

            insert_all_values(base_node, node.second, local_insertion_path);
        }
    }
}

/*------------------------------------------------------------------------------------------------*/
// sf::Vector2<Num>:

template<typename Num>
struct convert<sf::Vector2<Num>>
{
    // Expects a map that consists of:
    // ====================
    // * x: <typename Num>
    // * y: <typename Num>
    // ====================
    static bool decode(const Node& node, sf::Vector2<Num>& v2)
    {
        if (!node.IsDefined())
        {
            LOG_ALERT("undefined node.");
            return false;
        }

        try
        {
            v2.x = node["x"].as<Num>();
            v2.y = node["y"].as<Num>();
        }
        catch (const Exception& e)
        {
            LOG_ALERT("exception: " + e.what() + '\n' +
                      "invalid node; expected a map that consists of:\n"
                      "====================\n"
                      "* x: <typename Num>\n"
                      "* y: <typename Num>\n"
                      "====================\n"
                      "DUMP:\n" + Dump(node));
            return false;
        }

        return true;
    }

    // Returns a map that consists of:
    // ====================
    // * x: <typename Num>
    // * y: <typename Num>
    // ====================
    static Node encode(const sf::Vector2<Num>& v2)
    {
        Node node;
        node["x"] = v2.x;
        node["y"] = v2.y;
        return node;
    }
};

/*------------------------------------------------------------------------------------------------*/
// sf::Color:

template<>
struct convert<sf::Color>
{
    // Expects a map that consists of:
    // ==========================
    // * r: <unsigned int> = 0
    // * g: <unsigned int> = 0
    // * b: <unsigned int> = 0
    // * a: <unsigned int> = 255
    // ==========================
    static bool decode(const Node& node, sf::Color& color)
    {
        if (!node.IsDefined())
        {
            LOG_ALERT("undefined node.");
            return false;
        }

        try
        {
            const Node r_node = node["r"];
            const Node g_node = node["g"];
            const Node b_node = node["b"];
            const Node a_node = node["a"];
            
            color.r = r_node.IsDefined() ? static_cast<sf::Uint8>(r_node.as<unsigned int>()) : 0u;
            color.g = g_node.IsDefined() ? static_cast<sf::Uint8>(g_node.as<unsigned int>()) : 0u;
            color.b = b_node.IsDefined() ? static_cast<sf::Uint8>(b_node.as<unsigned int>()) : 0u;
            color.a = a_node.IsDefined() ? static_cast<sf::Uint8>(a_node.as<unsigned int>()) : 255u;
        }
        catch (const Exception& e)
        {
            LOG_ALERT("exception: " + e.what() + '\n' +
                      "invalid node; expected a map that consists of:\n"
                      "==========================\n"
                      "* r: <unsigned int> = 0\n"
                      "* g: <unsigned int> = 0\n"
                      "* b: <unsigned int> = 0\n"
                      "* a: <unsigned int> = 255\n"
                      "==========================\n"
                      "DUMP:\n" + Dump(node));
            return false;
        }

        return true;
    }

    // Returns a map that consists of:
    // ====================
    // * r: <unsigned int>
    // * g: <unsigned int>
    // * b: <unsigned int>
    // * a: <unsigned int>
    // ====================
    static Node encode(const sf::Color& color)
    {
        Node node;
        node["r"] = static_cast<unsigned int>(color.r);
        node["g"] = static_cast<unsigned int>(color.g);
        node["b"] = static_cast<unsigned int>(color.b);
        node["a"] = static_cast<unsigned int>(color.a);
        return node;
    }
};
}