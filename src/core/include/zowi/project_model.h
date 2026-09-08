#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace zowi {

// Quiz content is locale-specific and lives outside project.json (per-project
// quiz/<locale>.json files), so the core Project model carries only the
// metadata referenced by translation keys and firmware/achievement info.
struct Project {
    std::string id;
    std::string titleKey;
    std::string descriptionKey;
    std::string imageKey;
    std::string urlKey;
    std::string hexPath;
    std::string achievementId;
};

std::optional<Project> parseProjectJson(const nlohmann::json &json);

} // namespace zowi