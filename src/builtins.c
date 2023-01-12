#include <dc_env/env.h>
#include <dc_error/error.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include "command.h"

void builtin_cd(const struct dc_env *env, struct dc_error *err, struct command *command) {
    char path[1024];
    char home[1024];
    snprintf(home, sizeof(home), "%s", getenv("HOME"));
    if (command->argv[1] == NULL) {
        snprintf(path, sizeof(path), "%s", home);
    } else {
        snprintf(path, sizeof(path), "%s", command->argv[1]);
    }
    char *expanded_path = realpath(path, NULL);
    int ret_val = chdir(expanded_path);
    if (ret_val == -1) {
        command->exit_code = 1;
        switch (errno) {
            case EACCES:
                printf("EACCES: %s: %s\n", expanded_path, strerror(errno));
                break;

            case ELOOP:
                printf("ELOOP: %s: %s\n", expanded_path, strerror(errno));
                break;

            case ENAMETOOLONG:
                printf("ENAMETOOLONG: %s: %s\n", expanded_path, strerror(errno));
                break;

            case ENOENT:
                printf("ENOTDIR: %s: does not exist\n", expanded_path);
                break;

            case ENOTDIR:
                printf("ENOTDIR: %s: is not a directory\n", expanded_path);
                break;

            default:
                break;
        }
    }
}