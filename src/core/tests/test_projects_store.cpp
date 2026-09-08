#include <zowi/projects_store.h>
#include <cassert>
#include <iostream>
#include <fstream>
#include <cstdio>
#include <filesystem>

namespace fs = std::filesystem;

static std::string tmpBase() {
    static int counter = 0;
    return std::string(std::tmpnam(nullptr)) + "_proj_" + std::to_string(counter++);
}

void test_load_empty_index() {
    std::cout << "test_load_empty_index: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects");

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    assert(store.getAllProjects().empty());

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_load_index_bundles_projects() {
    std::cout << "test_load_index_bundles_projects: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects/move");

    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["move"] })";
    }
    {
        std::ofstream f(dir + "/projects/move/project.json");
        f << R"({
            "id": "move",
            "title_key": "title",
            "description_key": "learning_description",
            "image_key": "qrc:/images/projects/move_thumb.png",
            "url_key": "url",
            "hex_path": "",
            "achievement_id": "flapping"
        })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    auto proj = store.getProject("move");
    assert(proj.has_value());
    assert(proj->id == "move");
    assert(proj->titleKey == "title");
    assert(proj->descriptionKey == "learning_description");
    assert(proj->imageKey == "qrc:/images/projects/move_thumb.png");
    assert(proj->urlKey == "url");
    assert(proj->hexPath == "");
    assert(proj->achievementId == "flapping");

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_load_missing_project_survives() {
    std::cout << "test_load_missing_project_survives: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects");

    // index lists a project that has no project.json; should be skipped.
    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["ghost"] })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    assert(store.getAllProjects().empty());

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

int main() {
    test_load_empty_index();
    test_load_index_bundles_projects();
    test_load_missing_project_survives();

    std::cout << "\nAll ProjectsStore tests passed!" << std::endl;
    return 0;
}