#pragma once

#include <functional>
#include <string>
#include <vector>
#include <nlohmann/json.hpp>

namespace zowi {

// Where the optional configuration layers come from. Empty paths use the
// platform defaults; `getEnv` defaults to std::getenv. Tests inject all three.
struct ConfigLayers {
    std::string systemPath;
    std::string userPath;
    std::function<std::string(const std::string &name)> getEnv;
};

class ConfigStore {
public:
    explicit ConfigStore(const std::string &jsonPath);
    ConfigStore() : m_data(nlohmann::json::object()) {}

    void loadFromString(const std::string &jsonStr);
    std::string get(const std::string &key) const;
    bool has(const std::string &key) const;
    std::vector<std::string> keys() const;

    // Boolean view of a key: "true"/"false"/"1"/"0" (any case), default otherwise.
    bool getBool(const std::string &key, bool defaultValue = false) const;
    static bool parseBool(const std::string &value, bool defaultValue = false);

    // Merges a JSON object over the current values (keys of the new object win,
    // the others stay). Booleans and numbers are stored as text, like the rest
    // of the config. Returns false when the JSON is not an object.
    bool mergeFromString(const std::string &jsonStr);
    // Same, from a file; a missing or invalid file is ignored (returns false).
    bool mergeFromFile(const std::string &path);

    // Applies the override layers on top of the values already loaded (the
    // compiled config.json). From lowest to highest priority:
    //   user file   (only if allow_user_config is true)
    //   environment (only if allow_environment_config is true): ZOWI_<KEY>
    //   system file (always wins; sets the allow_* switches)
    // Each key is resolved on its own: a layer that does not define a key
    // leaves the value of the layer below. The allow_* switches are read only
    // from the system file or the compiled config, never from user/environment.
    void applyOverrides(const ConfigLayers &layers = {});

    static std::string defaultSystemConfigPath();  // /etc/ZowiDesktop/config.json
    static std::string defaultUserConfigPath();    // <config dir>/ZowiDesktop/config.json
    static std::string envNameFor(const std::string &key);  // "ranking_enabled" -> ZOWI_RANKING_ENABLED

private:
    nlohmann::json m_data;
};

} // namespace zowi
