#include <dc_error/error.h>
#include <dc_env/env.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include "state.h"
#include "command.h"
#include "builtins.h"
#include "shell.h"
#include "execute.h"
#include "util.h"

int init_state(const struct dc_env *env, struct dc_error *err, void *arg){
    struct state *state = arg;
    state->fatal_error = 0;
    state->max_line_length = sysconf(_SC_ARG_MAX);
    strcpy((char *) state->in_redirect_regex, "[ \t\f\v]<.*");
    strcpy((char *) state->out_redirect_regex, "[ \t\f\v][1^2]?>[>]?.*");
    strcpy((char *) state->err_redirect_regex, "[ \t\f\v]2>[>]?.*");
    state->path = NULL;

    char *path_env = getenv("PATH");
    if (path_env != NULL){
        state->path = malloc(sizeof (char *) * strlen(path_env));
        char *token = strtok(path_env, ":");
        int i = 0;
        while (token != NULL){
            state->path[i++] = token;
            token = strtok(NULL, ":");
        }
    }

    char *ps1_env = getenv("PS1");
    if (ps1_env == NULL){
        strcpy(state->prompt, "$ ");
    } else
    {
        strcpy(state->prompt, ps1_env);
    }
    return READ_COMMANDS;
}


int read_commands(const struct dc_env *env, struct dc_error *err, void *arg){
    struct state *state = arg;
    state->fatal_error = 0;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL){
        state->fatal_error = 1;
        return ERROR;
    }

    printf("%s %s", cwd, state->prompt);
    if (fgets(state->current_line, 1000, stdin) == NULL){
        state->fatal_error = 1;
        return ERROR;
    }

    state->current_line_length = strlen(state->current_line);
    if (state->current_line_length == 1){
        return RESET_STATE;
    }
    return SEPARATE_COMMANDS;
}

int separate_commands(const struct dc_env *env, struct dc_error *err, void *arg){
    struct state *state = arg;
    state->fatal_error = 0;
    strcpy(state->command->command, "");
    strcpy(state->command->command, state->current_line);

    // Initialize other fields here
    // zero out other fields

    return PARSE_COMMANDS;
}

int parse_commands(const struct dc_env *env, struct dc_error *err, void *arg){
    struct state *state = arg;
    state->fatal_error = 0;
    int parse_command_val = parse_command(env, err, state, state->command);
    if (parse_command_val){
        state->fatal_error = 1;
        return ERROR;
    }
    return EXECUTE_COMMANDS;
}

int execute_commands(const struct dc_env *env,
                     struct dc_error *err, void *arg) {
    struct state *state;
    if (strcmp(state->command->command, "cd") == 0) {
        builtin_cd(env, err, (struct command *) state->command->command);
    } else if (strcmp(state->command->command, "exit") == 0) {
        return DC_FSM_EXIT;
    } else {
        // not sure
        execute(env, err, (struct command *) state->command->command, (char *) state->path);
//        state.exit_code = execute();
//        if (state.exit_code != 0) {
//            state.fatal_error = 1;
    }

    printf("Exit code: %d\n", state->command->exit_code);
    if (state->fatal_error) {
        return ERROR;
    }
    return RESET_STATE;
}
int do_exit(const struct dc_env *env, struct dc_error *err, void *arg, struct state *state){
    do_reset_state(env, err, state);
    return DESTROY_STATE;
}

int reset_state(const struct dc_env *env, struct dc_error *error, void *arg, struct state *state){
    do_reset_state(env, error, state);
    return READ_COMMANDS;
}

int handle_error(const struct dc_env *env, struct dc_error *err, void *arg, struct state *state){
    if(state->current_line == NULL)
    {
        // not sure of "state->current_line"
        printf("Internal error (%d) %s\n", state->command->exit_code, state->current_line);
    } else{
        printf("Internal error (%d) %s: %s\n",
               state->command->exit_code, state->command->command, state->current_line);    }
    if (state->fatal_error){
        return DESTROY_STATE;
    }
    return RESET_STATE;
}

int destroy_state(const struct dc_env *env, struct dc_error *err, void *arg, struct state *state){
    free(state);
    return DC_FSM_EXIT;
}


