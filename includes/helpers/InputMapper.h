#pragma once
#include <nlohmann/json.hpp>
#include <string>
#include <unordered_map>

/**
 * @brief Maps logical actions to physical keys via JSON config.
 */
class InputMapper
{
  public:
    InputMapper() = default;

    bool LoadConfig(const std::string &filepath);
    int GetKey(const std::string &action, int defaultKey = 0) const;

  private:
    std::unordered_map<std::string, int> keyMap;
};
