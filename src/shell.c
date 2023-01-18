#include <dc_env/env.h>
#include <dc_error/error.h>
#include <dc_fsm/fsm.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include "shell.h"
#include "shell_impl.h"

int run_shell(const struct dc_env *env, struct dc_error *err) {
    int ret_val;
    dc_env_tracer tracer;
    struct dc_fsm_info *fsm_info;
    static struct dc_fsm_transition transitions[] = {
            {DC_FSM_INIT,       INIT_STATE,        init_state},

            {INIT_STATE,        READ_COMMANDS,     read_commands},

            {INIT_STATE,        ERROR,             handle_error},

            {READ_COMMANDS,     RESET_STATE,       reset_state},

            {READ_COMMANDS,     SEPARATE_COMMANDS, separate_commands},

            {READ_COMMANDS,     ERROR,             handle_error},

            {SEPARATE_COMMANDS, PARSE_COMMANDS,    parse_commands},

            {SEPARATE_COMMANDS, ERROR,             handle_error},

            {PARSE_COMMANDS,    EXECUTE_COMMANDS,  execute_commands},

            {PARSE_COMMANDS,    ERROR,             handle_error},

            {EXECUTE_COMMANDS,  RESET_STATE,       reset_state},

            {EXECUTE_COMMANDS,  EXIT,              do_exit},

            {EXECUTE_COMMANDS,  ERROR,             handle_error},

            {RESET_STATE,       READ_COMMANDS,     read_commands},

            {EXIT,              DESTROY_STATE,     destroy_state},

            {ERROR,             RESET_STATE,       reset_state},

            {ERROR,             DESTROY_STATE,     destroy_state},

            {DESTROY_STATE,     DC_FSM_EXIT, NULL},
    };
    dc_error_init(err, false);
    ret_val = EXIT_SUCCESS;
    fsm_info = dc_fsm_info_create(env, err, "shell");
    if (dc_error_has_no_error(err)) {
        int from_state, to_state;
        struct state state;
        ret_val = dc_fsm_run(env, err, fsm_info, &from_state, &to_state, &state, transitions);
        dc_fsm_info_destroy(env, &fsm_info);
    }
    return ret_val;
}

int run(const struct dc_env *env, struct dc_error *err, struct command *command, char **path) {

    if (strstr(command->command, "/") != NULL) {
        command->argv[0] = command->command;
        execve(command->command, command->argv, NULL);
    } else {
        if (path[0] == NULL) {
            DC_ERROR_RAISE_CHECK(err);
            fprintf(stderr, "Error: %s\n", strerror(ENOENT));
        } else {
            for (char * new_com = *path; new_com; new_com = *path++) {
                strcat(new_com, "/");
                strcat(new_com, command->command);

                command->argv[0] = new_com;
                execv(new_com, command->argv);
                if (dc_error_has_error(err)){
                    if (!dc_error_is_errno(err, ENOENT))
                        break;
                }
            }
        }
    }
}

