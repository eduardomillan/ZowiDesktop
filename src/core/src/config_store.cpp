#include <zowi/config_store.h>

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <set>

namespace zowi {

namespace {

constexpr const char *kAllowEnvKey = "allow_environment_config";
constexpr const char *kAllowUserKey = "allow_user_config";

bool isAllowKey(const std::string &key)
{
    return key == kAllowEnvKey || key == kAllowUserKey;
}

// Reads a JSON object from a file; empty object when missing or invalid.
nlohmann::json readObject(const std::string &path)
{
    std::ifstream file(path);
    if (!file.is_open()) return nlohmann::json::object();
    try {
        auto parsed = nlohmann::json::parse(file);
        if (parsed.is_object()) return parsed;
        std::cerr << "[config] Ignoring " << path << ": not a JSON object" << std::endl;
    } catch (...) {
        std::cerr << "[config] Ignoring " << path << ": invalid JSON" << std::endl;
    }
    return nlohmann::json::object();
}

// Text form of a value (config values are strings; booleans and numbers from
// override files are converted). Other types are skipped.
bool toText(const nlohmann::json &value, std::string &out)
{
    if (value.is_string()) { out = value.get<std::string>(); return true; }
    if (value.is_boolean()) { out = value.get<bool>() ? "true" : "false"; return true; }
    if (value.is_number()) { out = value.dump(); return true; }
    return false;
}

// Copies the usable entries of `source` into `target`, optionally skipping the
// allow_* switches.
void mergeObject(nlohmann::json &target, const nlohmann::json &source, bool skipAllowKeys)
{
    for (auto it = source.begin(); it != source.end(); ++it) {
        if (skipAllowKeys && isAllowKey(it.key())) continue;
        std::string text;
        if (toText(it.value(), text)) target[it.key()] = text;
    }
}

std::string lowerCopy(std::string s)
{
    std::transform(s.begin(), s.end(), s.begin(), [](unsigned char c) { return std::tolower(c); });
    return s;
}

std::string homeDir()
{
    const char *home = std::getenv("HOME");
    return home ? home : ".";
}

} // namespace

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

bool ConfigStore::parseBool(const std::string &value, bool defaultValue) {
    const std::string v = lowerCopy(value);
    if (v == "true" || v == "1") return true;
    if (v == "false" || v == "0") return false;
    return defaultValue;
}

bool ConfigStore::getBool(const std::string &key, bool defaultValue) const {
    return parseBool(get(key), defaultValue);
}

bool ConfigStore::mergeFromString(const std::string &jsonStr) {
    try {
        const auto parsed = nlohmann::json::parse(jsonStr);
        if (!parsed.is_object()) return false;
        if (!m_data.is_object()) m_data = nlohmann::json::object();
        mergeObject(m_data, parsed, false);
        return true;
    } catch (...) {
        return false;
    }
}

bool ConfigStore::mergeFromFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;
    try {
        const auto parsed = nlohmann::json::parse(file);
        if (!parsed.is_object()) return false;
        if (!m_data.is_object()) m_data = nlohmann::json::object();
        mergeObject(m_data, parsed, false);
        return true;
    } catch (...) {
        return false;
    }
}

std::string ConfigStore::defaultSystemConfigPath() {
#ifdef _WIN32
    const char *programData = std::getenv("PROGRAMDATA");
    return std::string(programData ? programData : "C:\\ProgramData") + "\\ZowiDesktop\\config.json";
#else
    return "/etc/ZowiDesktop/config.json";
#endif
}

std::string ConfigStore::defaultUserConfigPath() {
#ifdef _WIN32
    const char *appdata = std::getenv("APPDATA");
    return std::string(appdata ? appdata : ".") + "\\ZowiDesktop\\config.json";
#elif __APPLE__
    return homeDir() + "/Library/Application Support/ZowiDesktop/config.json";
#else
    const char *xdg = std::getenv("XDG_CONFIG_HOME");
    const std::string base = (xdg && *xdg) ? std::string(xdg) : homeDir() + "/.config";
    return base + "/ZowiDesktop/config.json";
#endif
}

std::string ConfigStore::envNameFor(const std::string &key) {
    std::string name = "ZOWI_";
    for (const char c : key) name += static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    return name;
}

void ConfigStore::applyOverrides(const ConfigLayers &layers) {
    if (!m_data.is_object()) m_data = nlohmann::json::object();

    const std::string systemPath = layers.systemPath.empty() ? defaultSystemConfigPath() : layers.systemPath;
    const std::string userPath = layers.userPath.empty() ? defaultUserConfigPath() : layers.userPath;
    const auto getEnv = layers.getEnv ? layers.getEnv : [](const std::string &name) {
        const char *v = std::getenv(name.c_str());
        return std::string(v ? v : "");
    };

    // The system file wins and is the only place (besides the compiled
    // defaults) that decides whether the lower layers are honoured.
    nlohmann::json system = nlohmann::json::object();
    mergeObject(system, readObject(systemPath), false);

    auto switchValue = [&](const char *key) {
        const std::string fromSystem = system.contains(key) ? system[key].get<std::string>() : std::string();
        return parseBool(fromSystem.empty() ? get(key) : fromSystem, false);
    };
    const bool allowUser = switchValue(kAllowUserKey);
    const bool allowEnv = switchValue(kAllowEnvKey);

    if (allowUser) mergeObject(m_data, readObject(userPath), true);

    if (allowEnv) {
        std::set<std::string> candidates;
        for (auto it = m_data.begin(); it != m_data.end(); ++it) candidates.insert(it.key());
        for (auto it = system.begin(); it != system.end(); ++it) candidates.insert(it.key());
        for (const auto &key : candidates) {
            if (isAllowKey(key)) continue;
            const std::string value = getEnv(envNameFor(key));
            if (!value.empty()) m_data[key] = value;
        }
    }

    for (auto it = system.begin(); it != system.end(); ++it) m_data[it.key()] = it.value();
}

} // namespace zowi
