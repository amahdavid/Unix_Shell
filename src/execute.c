#include <dc_env/env.h>
#include <dc_error/error.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdio.h>
#include "command.h"
#include "execute.h"
#include "shell.h"
#include "shell_impl.h"

void execute(const struct dc_env *env, struct dc_error *err, struct command *command, char *path) {
    pid_t child_pid = fork();
    if (child_pid == 0) {
        redirect(env, err, command);
        if (!dc_error_has_error(err)) {
            exit(126);
        }
        int status = run(env, err, command, path);
        if (status != EXIT_SUCCESS) {
            handle_run_error(err, command->command);
            printf("execute function in execute.c");
            exit(status);
        }
        execv(command->command, command->argv);
        exit(126);
    } else {
        int exit_val;
        waitpid(child_pid, &exit_val, 0);
        command->exit_code = exit_val;
    }
}