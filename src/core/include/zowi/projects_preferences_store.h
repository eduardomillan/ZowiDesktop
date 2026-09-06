#pragma once

#include <string>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>

namespace zowi {

class ProjectsPreferencesStore {
public:
    explicit ProjectsPreferencesStore(const std::string &configDir = "");

    int getBlockadeDurationMs() const;
    void setBlockadeDurationMs(int ms);

    bool isAchievementsEnabled() const;
    void setAchievementsEnabled(bool enabled);

    bool isQuizEnabled() const;
    void setQuizEnabled(bool enabled);

    nlohmann::json getProjectOverrides(const std::string &projectId) const;
    void setProjectOverride(const std::string &projectId, const nlohmann::json &override);

    using ChangedCallback = std::function<void()>;
    void onChanged(ChangedCallback cb) { m_onChanged = std::move(cb); }

private:
    void load();
    void save();
    static std::string resolveConfigPath(const std::string &configDir);

    std::string m_filePath;
    nlohmann::json m_data;
    ChangedCallback m_onChanged;
};

} // namespace zowi