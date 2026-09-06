#pragma once

#include <string>
#include <vector>
#include <optional>
#include <nlohmann/json.hpp>

namespace zowi {

struct ProjectAnswer {
    std::string textKey;
    bool correct = false;
};

struct ProjectQuestion {
    std::string textKey;
    std::vector<ProjectAnswer> answers;
};

struct Project {
    std::string id;
    std::string titleKey;
    std::string descriptionKey;
    std::string imageKey;
    std::string urlKey;
    std::vector<ProjectQuestion> questions;
    std::string hexPath;
    std::string achievementId;
};

std::optional<Project> parseProjectJson(const nlohmann::json &json);

} // namespace zowi