#include <zowi/projects_store.h>
#include <zowi/project_model.h>
#include <fstream>
#include <sstream>
#include <filesystem>

namespace zowi {

namespace {
std::string readFile(const std::string &path) {
    std::ifstream file(path);
    if (!file.is_open())
        return {};
    std::ostringstream ss;
    ss << file.rdbuf();
    return ss.str();
}
} // namespace

ProjectsStore::ProjectsStore()
    : m_loader([this]() { return defaultLoader(); })
{
}

// Default loader: reads projects/index.json (a JSON object with a "projects"
// array of project ids) and bundles every projects/<id>/project.json into a
// single JSON array. Falls back gracefully when the index is unavailable.
std::string ProjectsStore::defaultLoader() const {
    std::string base = m_resourceBasePath;
    if (!base.empty() && base.back() != '/')
        base += '/';
    base += "projects/";

    std::string indexContent = readFile(base + "index.json");
    if (indexContent.empty()) {
        if (m_log) m_log(LogLevel::Warning, "ProjectsStore: could not open " + base + "index.json");
        return "";
    }

    nlohmann::json index;
    try {
        index = nlohmann::json::parse(indexContent);
    } catch (const std::exception &) {
        if (m_log) m_log(LogLevel::Warning, "ProjectsStore: invalid index.json");
        return "";
    }

    nlohmann::json bundle = nlohmann::json::array();
    if (index.is_object() && index.contains("projects") && index["projects"].is_array()) {
        for (const auto &id : index["projects"]) {
            if (!id.is_string())
                continue;
            const std::string path = base + id.get<std::string>() + "/project.json";
            std::string content = readFile(path);
            if (content.empty()) {
                if (m_log) m_log(LogLevel::Warning, "ProjectsStore: could not open " + path);
                continue;
            }
            try {
                bundle.push_back(nlohmann::json::parse(content));
            } catch (const std::exception &) {
                if (m_log) m_log(LogLevel::Warning, "ProjectsStore: invalid " + path);
            }
        }
    }
    return bundle.dump();
}

void ProjectsStore::loadAll() {
    std::string jsonStr = m_loader ? m_loader() : defaultLoader();
    if (jsonStr.empty()) {
        if (m_log) m_log(LogLevel::Warning, "ProjectsStore: empty JSON, no projects loaded");
        m_projects.clear();
        if (m_onChanged) m_onChanged();
        return;
    }

    try {
        auto json = nlohmann::json::parse(jsonStr);
        m_projects.clear();

        if (json.is_array()) {
            for (const auto &item : json) {
                if (auto proj = parseProjectJson(item)) {
                    m_projects.push_back(std::move(*proj));
                }
            }
        } else if (json.is_object()) {
            if (auto proj = parseProjectJson(json)) {
                m_projects.push_back(std::move(*proj));
            }
        }
    } catch (const std::exception &e) {
        if (m_log) m_log(LogLevel::Warning, "ProjectsStore: JSON parse error: " + std::string(e.what()));
        m_projects.clear();
    }

    if (m_onChanged) m_onChanged();
}

std::optional<Project> ProjectsStore::getProject(const std::string &id) const {
    for (const auto &proj : m_projects) {
        if (proj.id == id)
            return proj;
    }
    return std::nullopt;
}

std::vector<Project> ProjectsStore::getAllProjects() const {
    return m_projects;
}

std::optional<Project> parseProjectJson(const nlohmann::json &json) {
    if (!json.is_object()) return std::nullopt;

    Project proj;
    try {
        proj.id = json.value("id", "");
        if (proj.id.empty()) return std::nullopt;

        proj.titleKey = json.value("title_key", "");
        proj.descriptionKey = json.value("description_key", "");
        proj.imageKey = json.value("image_key", "");
        proj.urlKey = json.value("url_key", "");
        proj.hexPath = json.value("hex_path", "");
        proj.achievementId = json.value("achievement_id", "");
        proj.actionTarget = json.value("action_target", "");
    } catch (...) {
        return std::nullopt;
    }

    return proj;
}

} // namespace zowi