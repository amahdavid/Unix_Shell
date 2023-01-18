#include "command.h"
#include "execute.h"
#include "shell.h"
#include "shell_impl.h"

void execute(const struct dc_env *env, struct dc_error *err, struct state *state, char **path) {

    pid_t child_pid = fork();
    int status;

    if (child_pid == 0) {
        redirect(env, err, state);
        if (dc_error_has_error(err)) {
            exit(126);
        }

        run(env, err, state->command, path);
        status = handle_run_error(env, err, state->command->command);
        if (status != EXIT_SUCCESS) {
             exit(status);
        }
        dc_execv(env, err, state->command->command, state->command->argv);
        exit(126);

    } else {
        int exit_val;
        waitpid(child_pid, &exit_val, 0);
        state->command->exit_code = exit_val;
    }
}