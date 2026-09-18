#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__ANDROID__)
    #define EMBER_PLATFORM "Android"
#elif defined(__linux__)
    #define EMBER_PLATFORM "Linux"
#else
    #define EMBER_PLATFORM "Unknown"
#endif

#define SETUP_MAX_PATH 512

typedef struct {
    char home_dir[SETUP_MAX_PATH];
    char config_dir[SETUP_MAX_PATH];
    char cache_dir[SETUP_MAX_PATH];
    int is_android;
} setup_env_t;

static void setup_strcpy(char* dest, const char* src, size_t max) {
    size_t i = 0;
    while (src[i] && i < max - 1) {
        dest[i] = src[i];
        i++;
    }
    dest[i] = '\0';
}

static int setup_dir_exists(const char* path) {
    FILE* test = fopen(path, "r");
    if (test) {
        fclose(test);
        return 1;
    }
    return 0;
}

static int setup_create_dir(const char* path) {
    char command[SETUP_MAX_PATH + 16];
    snprintf(command, sizeof(command), "mkdir -p \"%s\"", path);
    return system(command);
}

void setup_detect_environment(setup_env_t* env) {
    memset(env, 0, sizeof(setup_env_t));

#if defined(__ANDROID__)
    env->is_android = 1;
    setup_strcpy(env->home_dir, "/data/data/com.ember.os", SETUP_MAX_PATH);
    setup_strcpy(env->config_dir, "/data/data/com.ember.os/config", SETUP_MAX_PATH);
    setup_strcpy(env->cache_dir, "/data/data/com.ember.os/cache", SETUP_MAX_PATH);
#else
    env->is_android = 0;
    const char* home = getenv("HOME");
    if (!home) {
        home = "/tmp";
    }
    setup_strcpy(env->home_dir, home, SETUP_MAX_PATH);
    snprintf(env->config_dir, SETUP_MAX_PATH, "%s/.config/ember", home);
    snprintf(env->cache_dir, SETUP_MAX_PATH, "%s/.cache/ember", home);
#endif
}

int setup_check_tool(const char* tool_name) {
    char command[SETUP_MAX_PATH];
    snprintf(command, sizeof(command), "which %s > /dev/null 2>&1", tool_name);
    int result = system(command);
    return result == 0;
}

int setup_check_dependencies(void) {
    const char* required_tools[] = {
        "gcc",
        "nasm",
        "ld",
        "grub-mkrescue",
        "qemu-system-i386"
    };
    int tool_count = 5;
    int missing = 0;

    printf("Checking Ember OS build dependencies on %s\n", EMBER_PLATFORM);

    for (int i = 0; i < tool_count; i++) {
        int found = setup_check_tool(required_tools[i]);
        printf("  %-20s %s\n", required_tools[i], found ? "found" : "missing");
        if (!found) {
            missing++;
        }
    }

    return missing;
}

int setup_prepare_directories(setup_env_t* env) {
    if (!setup_dir_exists(env->config_dir)) {
        if (setup_create_dir(env->config_dir) != 0) {
            printf("Failed to create config directory: %s\n", env->config_dir);
            return -1;
        }
    }

    if (!setup_dir_exists(env->cache_dir)) {
        if (setup_create_dir(env->cache_dir) != 0) {
            printf("Failed to create cache directory: %s\n", env->cache_dir);
            return -1;
        }
    }

    return 0;
}

void setup_print_instructions(int missing_count) {
    if (missing_count == 0) {
        printf("\nAll dependencies satisfied. Ready to build Ember OS.\n");
        return;
    }

    printf("\n%d dependencies missing. Install with:\n", missing_count);

#if defined(__ANDROID__)
    printf("  pkg install gcc nasm binutils grub-efi qemu-system-x86\n");
#elif defined(__linux__)
    printf("  sudo apt install gcc nasm binutils grub-pc-bin xorriso qemu-system-x86\n");
#endif
}

int main(int argc, char** argv) {
    (void) argc;
    (void) argv;

    setup_env_t env;
    setup_detect_environment(&env);

    printf("Ember OS Setup\n");
    printf("Platform: %s\n", EMBER_PLATFORM);
    printf("Home: %s\n", env.home_dir);
    printf("Config: %s\n", env.config_dir);
    printf("Cache: %s\n\n", env.cache_dir);

    if (setup_prepare_directories(&env) != 0) {
        return 1;
    }

    int missing = setup_check_dependencies();
    setup_print_instructions(missing);

    return missing > 0 ? 1 : 0;
}
