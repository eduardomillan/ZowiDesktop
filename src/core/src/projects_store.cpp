#include <zowi/projects_store.h>
#include <zowi/project_model.h>
#include <fstream>
#include <filesystem>

namespace zowi {

ProjectsStore::ProjectsStore()
    : m_loader([this]() { return defaultLoader(); })
{
}

std::string ProjectsStore::defaultLoader() const {
    std::string path = m_resourceBasePath;
    if (!path.empty() && path.back() != '/')
        path += '/';
    path += "projects/move.json";
    std::ifstream file(path);
    if (file.is_open()) {
        std::string content((std::istreambuf_iterator<char>(file)),
                            std::istreambuf_iterator<char>());
        return content;
    }
    if (m_log) m_log(LogLevel::Warning, "ProjectsStore: could not open " + path);
    return "";
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

        if (json.contains("questions") && json["questions"].is_array()) {
            for (const auto &q : json["questions"]) {
                ProjectQuestion question;
                question.textKey = q.value("text_key", "");
                if (q.contains("answers") && q["answers"].is_array()) {
                    for (const auto &a : q["answers"]) {
                        ProjectAnswer ans;
                        ans.textKey = a.value("text_key", "");
                        ans.correct = a.value("correct", false);
                        question.answers.push_back(std::move(ans));
                    }
                }
                proj.questions.push_back(std::move(question));
            }
        }
    } catch (...) {
        return std::nullopt;
    }

    return proj;
}

} // namespace zowi