#include <string.h>
#include "state.h"
#include <dc_env/env.h>
#include <dc_error/error.h>
#include <malloc.h>
#include "shell.h"
char *get_path(const struct dc_env *env, struct dc_error *err)
{

}

const char *get_prompt(const struct dc_env *env, struct dc_error *err){

}

char **parse_path(const struct dc_env *env, struct dc_error *err, char *path_str){

}

void do_reset_state(const struct dc_env *env,
                    struct dc_error *err, struct state *state) {
    free(state->current_line);
    state->current_line = NULL;
    memset(err, 0, sizeof(state));
    // should return type be void or int?
    //return READ_COMMANDS;
}