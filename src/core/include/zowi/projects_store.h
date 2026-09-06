#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>
#include <nlohmann/json.hpp>
#include "project_model.h"

namespace zowi {

class ProjectsStore {
public:
    using ProjectsLoader = std::function<std::string()>;

    ProjectsStore();

    void setResourceBasePath(const std::string &path) { m_resourceBasePath = path; }
    void setProjectsLoader(ProjectsLoader loader) { m_loader = std::move(loader); }

    void loadAll();

    std::optional<Project> getProject(const std::string &id) const;
    std::vector<Project> getAllProjects() const;

    using ChangedCallback = std::function<void()>;
    void onChanged(ChangedCallback cb) { m_onChanged = std::move(cb); }

    // Diagnostics severity.
    enum class LogLevel { Info, Warning };
    using LogCallback = std::function<void(LogLevel level, const std::string &message)>;
    void setLogCallback(LogCallback cb) { m_log = std::move(cb); }

private:
    std::string defaultLoader() const;

    std::vector<Project> m_projects;
    std::string m_resourceBasePath;
    ProjectsLoader m_loader;
    LogCallback m_log;
    ChangedCallback m_onChanged;
};

} // namespace zowi