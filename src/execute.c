#include <dc_env/env.h>
#include <dc_error/error.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include "command.h"
#include "execute.h"
#include "shell.h"
#include "shell_impl.h"

void execute(const struct dc_env *env, struct dc_error *err, struct state * state, char ** path) {

    pid_t child_pid = fork();

    if (child_pid == 0) {
        redirect(env, err, state);
        if (dc_error_has_error(err)) {
            exit(126);
        }

        run(env, err, state->command, path);
        int status = handle_run_error(env, state);

        if (status != EXIT_SUCCESS) {
            handle_run_error(env, state->command->command);
            exit(status);
        }

        execv(state->command->command, state->command->argv);
        exit(126);

    } else {
        int exit_val;
        waitpid(child_pid, &exit_val, 0);
        state->command->exit_code = exit_val;
    }
}