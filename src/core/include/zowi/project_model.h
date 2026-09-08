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
    // Optional in-app navigation target for the project's action button
    // (e.g. "gamepad"). Empty = the action button is hidden. The GUI maps the
    // target to a concrete screen; the localized button label comes from the
    // project's strings/<locale>.json ("action_label").
    std::string actionTarget;
};

std::optional<Project> parseProjectJson(const nlohmann::json &json);

} // namespace zowi