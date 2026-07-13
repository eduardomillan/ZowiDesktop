#include <zowi/config_store.h>
#include <fstream>

namespace zowi {

ConfigStore::ConfigStore(const std::string &jsonPath) {
    std::ifstream file(jsonPath);
    if (file.is_open()) {
        try {
            m_data = nlohmann::json::parse(file);
        } catch (...) {
            m_data = nlohmann::json::object();
        }
    } else {
        m_data = nlohmann::json::object();
    }
}

void ConfigStore::loadFromString(const std::string &jsonStr) {
    try {
        m_data = nlohmann::json::parse(jsonStr);
    } catch (...) {
        m_data = nlohmann::json::object();
    }
}

std::string ConfigStore::get(const std::string &key) const {
    if (m_data.contains(key) && m_data[key].is_string())
        return m_data[key].get<std::string>();
    return "";
}

bool ConfigStore::has(const std::string &key) const {
    return m_data.contains(key);
}

std::vector<std::string> ConfigStore::keys() const {
    std::vector<std::string> result;
    if (m_data.is_object()) {
        for (auto it = m_data.begin(); it != m_data.end(); ++it) {
            result.push_back(it.key());
        }
    }
    return result;
}

} // namespace zowi
