#include <stddef.h>
#include <stdint.h>

#define EMBER_VERSION_MAJOR 0
#define EMBER_VERSION_MINOR 1
#define EMBER_VERSION_PATCH 0

#define EMBER_BRANCH "main"
#define EMBER_CODENAME "Ember"
#define EMBER_ARCH "x86_32"
#define EMBER_BUILD_DATE __DATE__
#define EMBER_BUILD_TIME __TIME__

#define PROJECT_MAX_FEATURES 32
#define PROJECT_MAX_MODULES 32

typedef enum {
    BUILD_STAGE_DEVELOP = 0,
    BUILD_STAGE_ALPHA = 1,
    BUILD_STAGE_BETA = 2,
    BUILD_STAGE_RC = 3,
    BUILD_STAGE_RELEASE = 4
} build_stage_t;

typedef enum {
    MODULE_STATE_UNINITIALIZED = 0,
    MODULE_STATE_INITIALIZING = 1,
    MODULE_STATE_READY = 2,
    MODULE_STATE_FAILED = 3
} module_state_t;

typedef struct {
    const char* name;
    module_state_t state;
} project_module_t;

typedef struct {
    int major;
    int minor;
    int patch;
    const char* branch;
    const char* codename;
    const char* arch;
    const char* build_date;
    const char* build_time;
    build_stage_t stage;
} project_info_t;

static const project_info_t project_info = {
    .major = EMBER_VERSION_MAJOR,
    .minor = EMBER_VERSION_MINOR,
    .patch = EMBER_VERSION_PATCH,
    .branch = EMBER_BRANCH,
    .codename = EMBER_CODENAME,
    .arch = EMBER_ARCH,
    .build_date = EMBER_BUILD_DATE,
    .build_time = EMBER_BUILD_TIME,
    .stage = BUILD_STAGE_DEVELOP
};

static const char* feature_names[PROJECT_MAX_FEATURES];
static int feature_count = 0;

static project_module_t modules[PROJECT_MAX_MODULES];
static int module_count = 0;

static int project_str_cmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] && b[i]) {
        if (a[i] != b[i]) {
            return a[i] - b[i];
        }
        i++;
    }
    return a[i] - b[i];
}

const project_info_t* project_get_info(void) {
    return &project_info;
}

int project_get_version_major(void) {
    return project_info.major;
}

int project_get_version_minor(void) {
    return project_info.minor;
}

int project_get_version_patch(void) {
    return project_info.patch;
}

const char* project_get_branch(void) {
    return project_info.branch;
}

const char* project_get_codename(void) {
    return project_info.codename;
}

const char* project_get_arch(void) {
    return project_info.arch;
}

const char* project_get_build_date(void) {
    return project_info.build_date;
}

const char* project_get_build_time(void) {
    return project_info.build_time;
}

build_stage_t project_get_stage(void) {
    return project_info.stage;
}

static const char* stage_to_string(build_stage_t stage) {
    switch (stage) {
        case BUILD_STAGE_DEVELOP: return "develop";
        case BUILD_STAGE_ALPHA: return "alpha";
        case BUILD_STAGE_BETA: return "beta";
        case BUILD_STAGE_RC: return "rc";
        case BUILD_STAGE_RELEASE: return "release";
        default: return "unknown";
    }
}

const char* project_get_stage_string(void) {
    return stage_to_string(project_info.stage);
}

int project_version_at_least(int major, int minor, int patch) {
    if (project_info.major != major) {
        return project_info.major > major;
    }
    if (project_info.minor != minor) {
        return project_info.minor > minor;
    }
    return project_info.patch >= patch;
}

int project_is_stable(void) {
    return project_info.stage == BUILD_STAGE_RELEASE;
}

void project_register_feature(const char* name) {
    if (feature_count >= PROJECT_MAX_FEATURES) {
        return;
    }
    feature_names[feature_count++] = name;
}

int project_has_feature(const char* name) {
    for (int i = 0; i < feature_count; i++) {
        if (project_str_cmp(feature_names[i], name) == 0) {
            return 1;
        }
    }
    return 0;
}

int project_get_feature_count(void) {
    return feature_count;
}

const char* project_get_feature(int index) {
    if (index < 0 || index >= feature_count) {
        return NULL;
    }
    return feature_names[index];
}

static int module_find(const char* name) {
    for (int i = 0; i < module_count; i++) {
        if (project_str_cmp(modules[i].name, name) == 0) {
            return i;
        }
    }
    return -1;
}

int project_register_module(const char* name) {
    if (module_count >= PROJECT_MAX_MODULES) {
        return -1;
    }

    if (module_find(name) >= 0) {
        return -1;
    }

    modules[module_count].name = name;
    modules[module_count].state = MODULE_STATE_UNINITIALIZED;
    module_count++;

    return module_count - 1;
}

int project_set_module_state(const char* name, module_state_t state) {
    int index = module_find(name);
    if (index < 0) {
        return -1;
    }
    modules[index].state = state;
    return 0;
}

module_state_t project_get_module_state(const char* name) {
    int index = module_find(name);
    if (index < 0) {
        return MODULE_STATE_UNINITIALIZED;
    }
    return modules[index].state;
}

int project_get_module_count(void) {
    return module_count;
}

const project_module_t* project_get_module(int index) {
    if (index < 0 || index >= module_count) {
        return NULL;
    }
    return &modules[index];
}

int project_all_modules_ready(void) {
    for (int i = 0; i < module_count; i++) {
        if (modules[i].state != MODULE_STATE_READY) {
            return 0;
        }
    }
    return module_count > 0;
}

int project_count_failed_modules(void) {
    int failed = 0;
    for (int i = 0; i < module_count; i++) {
        if (modules[i].state == MODULE_STATE_FAILED) {
            failed++;
        }
    }
    return failed;
}

void project_init(void) {
    feature_count = 0;
    module_count = 0;

    project_register_feature("vga_text_mode");
    project_register_feature("hardware_cursor");
    project_register_feature("gdt");
    project_register_feature("multiboot_meminfo");
    project_register_feature("ps2_keyboard");
    project_register_feature("pci_usb_detection");
    project_register_feature("path_utilities");
    project_register_feature("content_storage");

    project_register_module("terminal");
    project_register_module("gdt");
    project_register_module("keyboard");
    project_register_module("usb");
    project_register_module("paths");
    project_register_module("content");
}
