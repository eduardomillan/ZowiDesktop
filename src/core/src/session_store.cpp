#include <zowi/session_store.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>

namespace zowi {

SessionStore::SessionStore(const std::string &organization, const std::string &application)
    : m_filePath(resolveConfigPath(organization, application))
{
    load();
}

std::string SessionStore::getString(const std::string &key, const std::string &defaultValue) const {
    if (m_data.contains(key) && m_data[key].is_string())
        return m_data[key].get<std::string>();
    return defaultValue;
}

bool SessionStore::getBool(const std::string &key, bool defaultValue) const {
    if (m_data.contains(key) && m_data[key].is_boolean())
        return m_data[key].get<bool>();
    return defaultValue;
}

int SessionStore::getInt(const std::string &key, int defaultValue) const {
    if (m_data.contains(key) && m_data[key].is_number_integer())
        return m_data[key].get<int>();
    return defaultValue;
}

void SessionStore::setString(const std::string &key, const std::string &value) {
    m_data[key] = value;
    save();
    if (m_onChanged) m_onChanged();
}

void SessionStore::setBool(const std::string &key, bool value) {
    m_data[key] = value;
    save();
    if (m_onChanged) m_onChanged();
}

void SessionStore::setInt(const std::string &key, int value) {
    m_data[key] = value;
    save();
    if (m_onChanged) m_onChanged();
}

bool SessionStore::contains(const std::string &key) const {
    return m_data.contains(key);
}

std::string SessionStore::getRaw(const std::string &key) const {
    if (!m_data.contains(key)) return "";
    auto &val = m_data[key];
    if (val.is_string()) return val.get<std::string>();
    if (val.is_boolean()) return val.get<bool>() ? "true" : "false";
    if (val.is_number_integer()) return std::to_string(val.get<int>());
    if (val.is_number_float()) return std::to_string(val.get<double>());
    return val.dump();
}

std::vector<std::string> SessionStore::keys() const {
    std::vector<std::string> result;
    if (m_data.is_object()) {
        for (auto it = m_data.begin(); it != m_data.end(); ++it) {
            result.push_back(it.key());
        }
    }
    return result;
}

void SessionStore::load() {
    std::ifstream file(m_filePath);
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

void SessionStore::save() {
    std::ofstream file(m_filePath);
    if (file.is_open()) {
        file << m_data.dump(4);
    }
}

std::string SessionStore::resolveConfigPath(const std::string &org, const std::string &app) {
    std::filesystem::path dir;

#ifdef _WIN32
    const char *appdata = std::getenv("APPDATA");
    dir = appdata ? std::filesystem::path(appdata) : std::filesystem::path(".");
    dir /= org;
#elif __APPLE__
    const char *home = std::getenv("HOME");
    dir = home ? std::filesystem::path(home) / "Library/Application Support"
               : std::filesystem::path(".");
    dir /= org;
#else
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    const char *home = std::getenv("HOME");
    dir = xdg ? std::filesystem::path(xdg)
              : std::filesystem::path(home ? home : ".") / ".config";
    dir /= org;
#endif

    std::filesystem::create_directories(dir);
    return (dir / (app + ".json")).string();
}

} // namespace zowi
