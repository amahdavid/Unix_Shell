#ifndef DC_SHELL_SHELL_H
#define DC_SHELL_SHELL_H

#include <dc_env/env.h>
#include <dc_error/error.h>
#define DC_FSM_USER_START 2
enum shell_states
{
    INIT_STATE = DC_FSM_USER_START,    // 2
    READ_COMMANDS,
    ERROR,
    RESET_STATE,
    SEPARATE_COMMANDS,
    PARSE_COMMANDS,
    EXECUTE_COMMANDS,
    EXIT,
    DESTROY_STATE,
};

int run_shell(const struct dc_env *env, struct dc_error *err);

#endif //DC_SHELL_SHELL_H
