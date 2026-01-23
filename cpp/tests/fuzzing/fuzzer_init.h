#ifndef FUZZER_INIT_H
#define FUZZER_INIT_H

#include <poppler-global.h>
#include <unistd.h>
#include <libgen.h>
#include <linux/limits.h>
#include <string>

static void initialize_poppler_data_dir()
{
    static bool initialized = false;
    if (initialized)
        return;

    char exe_path[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exe_path, sizeof(exe_path) - 1);
    if (len != -1) {
        exe_path[len] = '\0';
        char *dir_path = dirname(exe_path);
        std::string data_dir = std::string(dir_path) + "/poppler-data";
        poppler::set_data_dir(data_dir);
    }

    initialized = true;
}

#endif // FUZZER_INIT_H
