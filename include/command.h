#ifndef DC_SHELL_COMMAND_H
#define DC_SHELL_COMMAND_H

#include <stdlib.h>
#include <stdbool.h>
#include <dc_error/error.h>
#include <dc_env/env.h>
#include "state.h"

// use calloc when checking sizeof command struct "struct command"
struct command {
    char *line;
    char *command;
    size_t argc;
    char **argv;
    char *stdin_file;
    char *stdout_file;
    bool stdout_overwrite;
    char *stderr_file;
    bool stderr_overwrite;
    int exit_code;
};

int parse_command(const struct dc_env *env, struct dc_error *err,
        struct state *state, struct command *command);

void redirect(const struct dc_env *env, const struct dc_error *err, struct command *command);


#endif //DC_SHELL_COMMAND_H
