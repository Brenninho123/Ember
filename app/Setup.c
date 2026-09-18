#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#if defined(__ANDROID__)
    #define EMBER_PLATFORM "Android"
#elif defined(__linux__)
    #define EMBER_PLATFORM "Linux"
#else
    #define EMBER_PLATFORM "Unknown"
#endif

#define SETUP_MAX_PATH 512
#define SETUP_MAX_TOOLS 16
#define SETUP_MAX_VERSION 64

typedef struct {
    char home_dir[SETUP_MAX_PATH];
    char config_dir[SETUP_MAX_PATH];
    char cache_dir[SETUP_MAX_PATH];
    char build_dir[SETUP_MAX_PATH];
    int is_android;
    int is_termux;
} setup_env_t;

typedef struct {
    const char* name;
    const char* min_version_flag;
    const char* apt_package;
    const char* termux_package;
    int required;
    int found;
    char version[SETUP_MAX_VERSION];
} setup_tool_t;

static setup_tool_t tools[SETUP_MAX_TOOLS] = {
    {"gcc",              "--version", "gcc",               "clang",              1, 0, ""},
    {"nasm",             "-v",        "nasm",               "nasm",               1, 0, ""},
    {"ld",               "--version", "binutils",           "binutils",           1, 0, ""},
    {"grub-mkrescue",    "--version", "grub-pc-bin",        "grub-efi",           1, 0, ""},
    {"xorriso",          "--version", "xorriso",            "xorriso",            1, 0, ""},
    {"mformat",          NULL,        "mtools",             "mtools",             1, 0, ""},
    {"qemu-system-i386", "--version", "qemu-system-x86",    "qemu-system-x86",    0, 0, ""},
    {"git",              "--version", "git",                "git",                0, 0, ""}
};

static int tool_count = 8;

static void setup_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int setup_dir_exists(const char* path) {
    return access(path, F_OK) == 0;
}

static int setup_create_dir(const char* path) {
    char command[SETUP_MAX_PATH + 16];
    snprintf(command, sizeof(command), "mkdir -p \"%s\"", path);
    return system(command);
}

static int setup_detect_termux(void) {
    return getenv("TERMUX_VERSION") != NULL || setup_dir_exists("/data/data/com.termux");
}

void setup_detect_environment(setup_env_t* env) {
    memset(env, 0, sizeof(setup_env_t));

#if defined(__ANDROID__)
    env->is_android = 1;
    env->is_termux = setup_detect_termux();

    if (env->is_termux) {
        const char* prefix = getenv("PREFIX");
        const char* home = getenv("HOME");
        setup_strcpy(env->home_dir, home ? home : "/data/data/com.termux/files/home", SETUP_MAX_PATH);
        snprintf(env->config_dir, SETUP_MAX_PATH, "%s/.config/ember", env->home_dir);
        snprintf(env->cache_dir, SETUP_MAX_PATH, "%s/.cache/ember", env->home_dir);
        (void) prefix;
    } else {
        setup_strcpy(env->home_dir, "/data/data/com.ember.os", SETUP_MAX_PATH);
        setup_strcpy(env->config_dir, "/data/data/com.ember.os/config", SETUP_MAX_PATH);
        setup_strcpy(env->cache_dir, "/data/data/com.ember.os/cache", SETUP_MAX_PATH);
    }
#else
    env->is_android = 0;
    env->is_termux = 0;
    const char* home = getenv("HOME");
    if (!home) {
        home = "/tmp";
    }
    setup_strcpy(env->home_dir, home, SETUP_MAX_PATH);
    snprintf(env->config_dir, SETUP_MAX_PATH, "%s/.config/ember", home);
    snprintf(env->cache_dir, SETUP_MAX_PATH, "%s/.cache/ember", home);
#endif

    snprintf(env->build_dir, SETUP_MAX_PATH, "%s/build", env->config_dir);
}

static int setup_run_capture(const char* command, char* output, size_t max_size) {
    FILE* pipe = popen(command, "r");
    if (!pipe) {
        return -1;
    }

    size_t total = 0;
    if (fgets(output, (int) max_size, pipe) != NULL) {
        total = strlen(output);
        if (total > 0 && output[total - 1] == '\n') {
            output[total - 1] = '\0';
        }
    }

    pclose(pipe);
    return (int) total;
}

static int setup_check_tool(setup_tool_t* tool) {
    char command[256];
    snprintf(command, sizeof(command), "command -v %s > /dev/null 2>&1", tool->name);

    int found = system(command) == 0;
    tool->found = found;

    if (found && tool->min_version_flag) {
        char version_command[256];
        snprintf(version_command, sizeof(version_command), "%s %s 2>&1", tool->name, tool->min_version_flag);
        setup_run_capture(version_command, tool->version, SETUP_MAX_VERSION);
    } else if (found) {
        setup_strcpy(tool->version, "present", SETUP_MAX_VERSION);
    } else {
        setup_strcpy(tool->version, "-", SETUP_MAX_VERSION);
    }

    return found;
}

int setup_check_dependencies(int is_termux) {
    (void) is_termux;
    int missing_required = 0;

    printf("Checking Ember OS build dependencies on %s\n\n", EMBER_PLATFORM);
    printf("%-20s %-10s %-30s\n", "Tool", "Status", "Version");
    printf("%-20s %-10s %-30s\n", "----", "------", "-------");

    for (int i = 0; i < tool_count; i++) {
        setup_check_tool(&tools[i]);

        const char* status = tools[i].found ? "found" : (tools[i].required ? "MISSING" : "optional");
        printf("%-20s %-10s %-30s\n", tools[i].name, status, tools[i].version);

        if (!tools[i].found && tools[i].required) {
            missing_required++;
        }
    }

    printf("\n");
    return missing_required;
}

int setup_prepare_directories(setup_env_t* env) {
    const char* dirs[] = { env->config_dir, env->cache_dir, env->build_dir };

    for (int i = 0; i < 3; i++) {
        if (!setup_dir_exists(dirs[i])) {
            if (setup_create_dir(dirs[i]) != 0) {
                printf("Failed to create directory: %s\n", dirs[i]);
                return -1;
            }
            printf("Created: %s\n", dirs[i]);
        }
    }

    return 0;
}

void setup_print_install_command(const setup_env_t* env) {
    printf("Install missing dependencies with:\n\n");

    if (env->is_termux) {
        printf("  pkg update && pkg install");
        for (int i = 0; i < tool_count; i++) {
            if (!tools[i].found && tools[i].required) {
                printf(" %s", tools[i].termux_package);
            }
        }
        printf("\n");
    } else if (env->is_android) {
        printf("  This environment does not support automatic dependency installation.\n");
        printf("  Consider running Ember Setup inside Termux instead.\n");
    } else {
        printf("  sudo apt update && sudo apt install");
        for (int i = 0; i < tool_count; i++) {
            if (!tools[i].found && tools[i].required) {
                printf(" %s", tools[i].apt_package);
            }
        }
        printf("\n");
    }
}

int setup_offer_auto_install(const setup_env_t* env, int missing_count) {
    if (missing_count == 0) {
        return 0;
    }

    if (env->is_android && !env->is_termux) {
        return -1;
    }

    printf("Attempt automatic installation now? [y/N] ");
    fflush(stdout);

    int response = getchar();
    if (response != 'y' && response != 'Y') {
        return 0;
    }

    char command[1024];
    command[0] = '\0';

    if (env->is_termux) {
        strcat(command, "pkg update && pkg install -y");
        for (int i = 0; i < tool_count; i++) {
            if (!tools[i].found && tools[i].required) {
                strcat(command, " ");
                strcat(command, tools[i].termux_package);
            }
        }
    } else {
        strcat(command, "sudo apt update && sudo apt install -y");
        for (int i = 0; i < tool_count; i++) {
            if (!tools[i].found && tools[i].required) {
                strcat(command, " ");
                strcat(command, tools[i].apt_package);
            }
        }
    }

    printf("\nRunning: %s\n\n", command);
    return system(command);
}

void setup_print_summary(int missing_count, const setup_env_t* env) {
    printf("Home:   %s\n", env->home_dir);
    printf("Config: %s\n", env->config_dir);
    printf("Cache:  %s\n", env->cache_dir);
    printf("Build:  %s\n\n", env->build_dir);

    if (missing_count == 0) {
        printf("All required dependencies satisfied. Ready to build Ember OS.\n");
    } else {
        printf("%d required dependencies missing.\n", missing_count);
    }
}

int main(int argc, char** argv) {
    int auto_install = 0;

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--auto-install") == 0) {
            auto_install = 1;
        }
    }

    setup_env_t env;
    setup_detect_environment(&env);

    printf("Ember OS Setup\n");
    printf("Platform: %s%s\n\n", EMBER_PLATFORM, env.is_termux ? " (Termux)" : "");

    if (setup_prepare_directories(&env) != 0) {
        return 1;
    }

    int missing = setup_check_dependencies(env.is_termux);

    if (missing > 0) {
        setup_print_install_command(&env);
        printf("\n");

        if (auto_install) {
            setup_offer_auto_install(&env, missing);
        }
    }

    setup_print_summary(missing, &env);

    return missing > 0 ? 1 : 0;
}
