#include "helpers/InputMapper.h"
#include <fstream>
#include <iostream>

bool InputMapper::LoadConfig(const std::string& filepath)
{
    std::ifstream file(filepath);
    if (!file.is_open())
    {
        std::cerr << "[InputMapper] Error: Could not open config file: " << filepath << "\n";
        return false;
    }

    try
    {
        nlohmann::json j;
        file >> j;
        
        for (auto& [key, value] : j.items())
        {
            keyMap[key] = value.get<int>();
        }
        
        std::cout << "[InputMapper] Loaded " << keyMap.size() << " keybindings from " << filepath << "\n";
        return true;
    }
    catch (const nlohmann::json::parse_error& e)
    {
        std::cerr << "[InputMapper] JSON parse error: " << e.what() << "\n";
        return false;
    }
}

int InputMapper::GetKey(const std::string& action, int defaultKey) const
{
    auto it = keyMap.find(action);
    if (it != keyMap.end())
    {
        return it->second;
    }
    return defaultKey;
}

