#include <zowi/projects_preferences_store.h>
#include <fstream>
#include <cstdlib>
#include <filesystem>

namespace zowi {

ProjectsPreferencesStore::ProjectsPreferencesStore(const std::string &configDir)
    : m_filePath(resolveConfigPath(configDir))
{
    load();
}

int ProjectsPreferencesStore::getBlockadeDurationMs() const {
    if (m_data.contains("blockade_duration_ms") && m_data["blockade_duration_ms"].is_number_integer())
        return m_data["blockade_duration_ms"].get<int>();
    return 600000; // 10 minutes default
}

void ProjectsPreferencesStore::setBlockadeDurationMs(int ms) {
    m_data["blockade_duration_ms"] = ms;
    save();
    if (m_onChanged) m_onChanged();
}

bool ProjectsPreferencesStore::isAchievementsEnabled() const {
    if (m_data.contains("achievements_enabled") && m_data["achievements_enabled"].is_boolean())
        return m_data["achievements_enabled"].get<bool>();
    return false;
}

void ProjectsPreferencesStore::setAchievementsEnabled(bool enabled) {
    m_data["achievements_enabled"] = enabled;
    save();
    if (m_onChanged) m_onChanged();
}

bool ProjectsPreferencesStore::isQuizEnabled() const {
    if (m_data.contains("quiz_enabled") && m_data["quiz_enabled"].is_boolean())
        return m_data["quiz_enabled"].get<bool>();
    return true;
}

void ProjectsPreferencesStore::setQuizEnabled(bool enabled) {
    m_data["quiz_enabled"] = enabled;
    save();
    if (m_onChanged) m_onChanged();
}

nlohmann::json ProjectsPreferencesStore::getProjectOverrides(const std::string &projectId) const {
    if (m_data.contains("projects") && m_data["projects"].contains(projectId))
        return m_data["projects"][projectId];
    return nlohmann::json::object();
}

void ProjectsPreferencesStore::setProjectOverride(const std::string &projectId, const nlohmann::json &override) {
    m_data["projects"][projectId] = override;
    save();
    if (m_onChanged) m_onChanged();
}

void ProjectsPreferencesStore::load() {
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

void ProjectsPreferencesStore::save() {
    std::ofstream file(m_filePath);
    if (file.is_open()) {
        file << m_data.dump(4);
    }
}

std::string ProjectsPreferencesStore::resolveConfigPath(const std::string &configDir) {
    std::filesystem::path dir;

    if (!configDir.empty()) {
        dir = configDir;
    } else {
#ifdef _WIN32
        const char *appdata = std::getenv("APPDATA");
        dir = appdata ? std::filesystem::path(appdata) : std::filesystem::path(".");
        dir /= "ZowiDesktop";
#elif __APPLE__
        const char *home = std::getenv("HOME");
        dir = home ? std::filesystem::path(home) / "Library/Application Support"
                   : std::filesystem::path(".");
        dir /= "ZowiDesktop";
#else
        const char *xdg = std::getenv("XDG_CONFIG_HOME");
        const char *home = std::getenv("HOME");
        dir = xdg ? std::filesystem::path(xdg)
                  : std::filesystem::path(home ? home : ".") / ".config";
        dir /= "ZowiDesktop";
#endif
    }

    std::filesystem::create_directories(dir);
    return (dir / "projects_preferences.json").string();
}

} // namespace zowi