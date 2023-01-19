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

        if (run(env, err, state->command, path) == -1){
            status = handle_run_error(env, err, state->command->command);
            exit(status);
        }

    } else {
        int exit_val;
        waitpid(child_pid, &exit_val, 0);
        state->command->exit_code = exit_val;
    }
}