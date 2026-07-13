#pragma once

#include <string>
#include <vector>
#include <functional>
#include <nlohmann/json.hpp>

namespace zowi {

class SessionStore {
public:
    explicit SessionStore(const std::string &organization = "ZowiDesktop",
                          const std::string &application = "ZowiApp");

    std::string getString(const std::string &key, const std::string &defaultValue = "") const;
    bool getBool(const std::string &key, bool defaultValue = false) const;
    int getInt(const std::string &key, int defaultValue = 0) const;

    void setString(const std::string &key, const std::string &value);
    void setBool(const std::string &key, bool value);
    void setInt(const std::string &key, int value);

    bool contains(const std::string &key) const;
    std::string getRaw(const std::string &key) const;
    std::vector<std::string> keys() const;

    using ChangedCallback = std::function<void()>;
    void onChanged(ChangedCallback cb) { m_onChanged = std::move(cb); }

private:
    std::string m_filePath;
    nlohmann::json m_data;
    ChangedCallback m_onChanged;

    void load();
    void save();
    static std::string resolveConfigPath(const std::string &org, const std::string &app);
};

} // namespace zowi
