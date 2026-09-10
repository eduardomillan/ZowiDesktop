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
            "achievement_id": "flapping",
            "action_target": "gamepad"
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
    assert(proj->actionTarget == "gamepad");

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

void test_load_bio3_project() {
    std::cout << "test_load_bio3_project: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects/bio3");

    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["bio3"] })";
    }
    {
        std::ofstream f(dir + "/projects/bio3/project.json");
        f << R"({
            "id": "bio3",
            "title_key": "title",
            "description_key": "learning_description",
            "image_key": "qrc:/images/projects/bio3_thumb.png",
            "url_key": "url",
            "hex_path": "",
            "achievement_id": "jitter"
        })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    auto proj = store.getProject("bio3");
    assert(proj.has_value());
    assert(proj->id == "bio3");
    assert(proj->titleKey == "title");
    assert(proj->descriptionKey == "learning_description");
    assert(proj->imageKey == "qrc:/images/projects/bio3_thumb.png");
    assert(proj->urlKey == "url");
    assert(proj->hexPath == "");
    assert(proj->achievementId == "jitter");

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_load_form_project() {
    std::cout << "test_load_form_project: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects/form");

    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["form"] })";
    }
    {
        std::ofstream f(dir + "/projects/form/project.json");
        f << R"({
            "id": "form",
            "title_key": "title",
            "description_key": "learning_description",
            "image_key": "qrc:/images/projects/form_thumb.jpg",
            "url_key": "url",
            "hex_path": "",
            "achievement_id": "confused"
        })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    auto proj = store.getProject("form");
    assert(proj.has_value());
    assert(proj->id == "form");
    assert(proj->titleKey == "title");
    assert(proj->descriptionKey == "learning_description");
    assert(proj->imageKey == "qrc:/images/projects/form_thumb.jpg");
    assert(proj->urlKey == "url");
    assert(proj->hexPath == "");
    assert(proj->achievementId == "confused");

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_load_choreography_project() {
    std::cout << "test_load_choreography_project: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects/choreography");

    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["choreography"] })";
    }
    {
        std::ofstream f(dir + "/projects/choreography/project.json");
        f << R"({
            "id": "choreography",
            "title_key": "title",
            "description_key": "learning_description",
            "image_key": "qrc:/images/projects/choreography_thumb.png",
            "url_key": "url",
            "hex_path": "",
            "achievement_id": "swing"
        })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    auto proj = store.getProject("choreography");
    assert(proj.has_value());
    assert(proj->id == "choreography");
    assert(proj->titleKey == "title");
    assert(proj->descriptionKey == "learning_description");
    assert(proj->imageKey == "qrc:/images/projects/choreography_thumb.png");
    assert(proj->urlKey == "url");
    assert(proj->hexPath == "");
    assert(proj->achievementId == "swing");
    assert(proj->actionTarget == ""); // not configured → default empty

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_load_bio1_project() {
    std::cout << "test_load_bio1_project: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects/bio1");

    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["bio1"] })";
    }
    {
        std::ofstream f(dir + "/projects/bio1/project.json");
        f << R"({
            "id": "bio1",
            "title_key": "title",
            "description_key": "learning_description",
            "image_key": "qrc:/images/projects/biology_thumb.jpg",
            "url_key": "url",
            "hex_path": "",
            "achievement_id": "wave"
        })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    auto proj = store.getProject("bio1");
    assert(proj.has_value());
    assert(proj->id == "bio1");
    assert(proj->titleKey == "title");
    assert(proj->descriptionKey == "learning_description");
    assert(proj->imageKey == "qrc:/images/projects/biology_thumb.jpg");
    assert(proj->urlKey == "url");
    assert(proj->hexPath == "");
    assert(proj->achievementId == "wave");
    assert(proj->actionTarget == ""); // not configured → default empty

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_load_reprogram_project() {
    std::cout << "test_load_reprogram_project: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects/reprogram");

    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["reprogram"] })";
    }
    {
        std::ofstream f(dir + "/projects/reprogram/project.json");
        f << R"({
            "id": "reprogram",
            "title_key": "title",
            "description_key": "learning_description",
            "image_key": "qrc:/images/projects/reprogram.jpg",
            "url_key": "url",
            "hex_path": "ZOWI_Alarm_v2.hex",
            "achievement_id": "angry"
        })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    auto proj = store.getProject("reprogram");
    assert(proj.has_value());
    assert(proj->id == "reprogram");
    assert(proj->titleKey == "title");
    assert(proj->descriptionKey == "learning_description");
    assert(proj->imageKey == "qrc:/images/projects/reprogram.jpg");
    assert(proj->urlKey == "url");
    assert(proj->hexPath == "ZOWI_Alarm_v2.hex");
    assert(proj->achievementId == "angry");
    assert(proj->actionTarget == ""); // not configured → default empty

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_load_adivinawi_project() {
    std::cout << "test_load_adivinawi_project: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects/adivinawi");

    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["adivinawi"] })";
    }
    {
        std::ofstream f(dir + "/projects/adivinawi/project.json");
        f << R"({
            "id": "adivinawi",
            "title_key": "title",
            "description_key": "learning_description",
            "image_key": "qrc:/images/projects/adivinawi.png",
            "url_key": "url",
            "hex_path": "ZOWI_Adivinawi_v2.hex",
            "achievement_id": "magic"
        })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    auto proj = store.getProject("adivinawi");
    assert(proj.has_value());
    assert(proj->id == "adivinawi");
    assert(proj->titleKey == "title");
    assert(proj->descriptionKey == "learning_description");
    assert(proj->imageKey == "qrc:/images/projects/adivinawi.png");
    assert(proj->urlKey == "url");
    assert(proj->hexPath == "ZOWI_Adivinawi_v2.hex");
    assert(proj->achievementId == "magic");
    assert(proj->actionTarget == ""); // not configured → default empty

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_load_helloworld_project() {
    std::cout << "test_load_helloworld_project: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects/helloworld");

    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["helloworld"] })";
    }
    {
        std::ofstream f(dir + "/projects/helloworld/project.json");
        f << R"({
            "id": "helloworld",
            "title_key": "title",
            "description_key": "learning_description",
            "image_key": "qrc:/images/android/project_helloworld_image.png",
            "url_key": "url",
            "hex_path": "",
            "achievement_id": "super_happy"
        })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    auto proj = store.getProject("helloworld");
    assert(proj.has_value());
    assert(proj->id == "helloworld");
    assert(proj->titleKey == "title");
    assert(proj->descriptionKey == "learning_description");
    assert(proj->imageKey == "qrc:/images/android/project_helloworld_image.png");
    assert(proj->urlKey == "url");
    assert(proj->hexPath == "");
    assert(proj->achievementId == "super_happy");
    assert(proj->actionTarget == ""); // not configured → default empty

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

void test_load_bitbloq2_project() {
    std::cout << "test_load_bitbloq2_project: " << std::flush;

    std::string dir = tmpBase();
    fs::create_directories(dir + "/projects/bitbloq2");

    {
        std::ofstream f(dir + "/projects/index.json");
        f << R"({ "projects": ["bitbloq2"] })";
    }
    {
        std::ofstream f(dir + "/projects/bitbloq2/project.json");
        f << R"({
            "id": "bitbloq2",
            "title_key": "title",
            "description_key": "learning_description",
            "image_key": "qrc:/images/android/project_bitbloq2_image.png",
            "url_key": "url",
            "hex_path": "",
            "achievement_id": "tip_toe"
        })";
    }

    zowi::ProjectsStore store;
    store.setResourceBasePath(dir);
    store.loadAll();

    auto proj = store.getProject("bitbloq2");
    assert(proj.has_value());
    assert(proj->id == "bitbloq2");
    assert(proj->titleKey == "title");
    assert(proj->descriptionKey == "learning_description");
    assert(proj->imageKey == "qrc:/images/android/project_bitbloq2_image.png");
    assert(proj->urlKey == "url");
    assert(proj->hexPath == "");
    assert(proj->achievementId == "tip_toe");
    assert(proj->actionTarget == ""); // not configured → default empty

    fs::remove_all(dir);
    std::cout << "OK" << std::endl;
}

int main() {
    test_load_empty_index();
    test_load_index_bundles_projects();
    test_load_bio3_project();
    test_load_form_project();
    test_load_bio1_project();
    test_load_choreography_project();
    test_load_reprogram_project();
    test_load_adivinawi_project();
    test_load_helloworld_project();
    test_load_bitbloq2_project();
    test_load_missing_project_survives();

    std::cout << "\nAll ProjectsStore tests passed!" << std::endl;
    return 0;
}