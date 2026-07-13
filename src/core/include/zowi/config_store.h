#pragma once

#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace zowi {

class ConfigStore {
public:
    explicit ConfigStore(const std::string &jsonPath);
    ConfigStore() : m_data(nlohmann::json::object()) {}

    void loadFromString(const std::string &jsonStr);
    std::string get(const std::string &key) const;
    bool has(const std::string &key) const;
    std::vector<std::string> keys() const;

private:
    nlohmann::json m_data;
};

} // namespace zowi
