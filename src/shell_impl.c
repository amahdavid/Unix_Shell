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
#include "shell_impl.h"
#include <dc_fsm/fsm.h>

#define BUF_SIZE 4000

typedef struct command command;
regex_t err_regex;
regex_t in_regex;
regex_t out_regex;

int init_state(const struct dc_env *env, struct dc_error *err, void *arg) {

    struct state *state = (struct state *) arg;
    state->fatal_error = 0;
    state->max_line_length = sysconf(_SC_ARG_MAX);

    regcomp(&in_regex, "[ \t\f\v]<.*", REG_EXTENDED);
    regcomp(&out_regex, "[ \t\f\v][1^2]?>[>]?.*", REG_EXTENDED);
    regcomp(&err_regex, "[ \t\f\v]2>[>]?.*", REG_EXTENDED);

    state->in_redirect_regex = &in_regex;
    state->out_redirect_regex = &out_regex;
    state->err_redirect_regex = &err_regex;
    state->path = NULL;

    if (err != NULL && dc_error_has_error(err)) {
        state->fatal_error = true;
        printf("DC HAS ERROR IN MAIN IF STATEMENT");
        return EXIT_FAILURE;
    }
    char *path_env = getenv("PATH");
    if (path_env != NULL) {
        size_t path_len = strlen(path_env);
        state->path = malloc(sizeof(char *) * (path_len + 1));
        if (state->path == NULL) {
            printf("error allocating memory for path: %s\n", strerror(errno));
            state->fatal_error = true;
            return EXIT_FAILURE;
        }
        char *token = strtok(path_env, ":");
        int i = 0;
        while (token != NULL) {
            state->path[i++] = token;
            token = strtok(NULL, ":");
        }
        state->path[i] = NULL;
    }

    //get_path(env, err);

    char *ps1_env = getenv("PS1");
    if (ps1_env == NULL) {
        state->prompt = (char *) malloc(sizeof(char) * 3);
        strcpy(state->prompt, "$ ");
    } else {
        size_t ps1_len = strlen(ps1_env);
        state->prompt = (char *) malloc(sizeof(char) * (ps1_len + 1));
        strcpy(state->prompt, ps1_env);
    }

    //get_prompt(env, err);
    return READ_COMMANDS;
}


int read_commands(const struct dc_env *env, struct dc_error *err, void *arg) {
    struct state *state = arg;
    state->fatal_error = 0;

    char cwd[1024];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        state->fatal_error = 1;
        return ERROR;
    }

    printf("%s %s", cwd, state->prompt);
    char *line = NULL;
    size_t n = 0;
    long read = getline(&line, &n, stdin);
    if (read == -1) {
        state->fatal_error = 1;
        return ERROR;
    }

    if (n > BUF_SIZE) {
        free(line);
        state->fatal_error = 1;
        return ERROR;
    }
    state->current_line = malloc(n);
    memcpy(state->current_line, line, n);
    free(line);

    state->current_line_length = strlen(state->current_line);
    if (state->current_line_length == 1) {
        return RESET_STATE;
    }
    return SEPARATE_COMMANDS;
}

int separate_commands(const struct dc_env *env, struct dc_error *err, void *arg) {

    struct state *state = arg;

    if (dc_error_has_error(err)) {
        state->fatal_error = true;
        return ERROR;
    }

    // THIS CAUSES A SIGABRT ERROR
//    if (state->command){
//        free(state->command);
//    }
    state->command = (command *) malloc(sizeof(command));
    if (state->command == NULL){
        return ERROR;
    }
    memset(state->command, 0, sizeof(command));
    size_t line_size = strlen(state->current_line) + 1;
    state->command->line = (char *) malloc(line_size);

    if (state->command->line == NULL){
        free(state->command);
        return ERROR;
    }

    strcpy(state->command->line, state->current_line);

    return PARSE_COMMANDS;
}

int parse_commands(const struct dc_env *env, struct dc_error *err, void *arg) {
    struct state *state = arg;
    state->fatal_error = 0;
    int parse_command_val = parse_command(env, err, state, state->command);
    if (parse_command_val) {
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

int do_exit(const struct dc_env *env, struct dc_error *err, void *arg) {
    struct state *state = arg;
    do_reset_state(env, err, state);
    return DESTROY_STATE;
}

int reset_state(const struct dc_env *env, struct dc_error *error, void *arg) {
    struct state *state = arg;
    do_reset_state(env, error, state);
    return READ_COMMANDS;
}

int handle_error(const struct dc_env *env, struct dc_error *err, void *arg) {
    struct state *state = arg;
    if (state->current_line == NULL) {
        // not sure of "state->current_line"
        printf("Internal error (%d) %s\n", state->command->exit_code, state->current_line);
    } else {
        printf("Internal error (%d) %s: %s\n",
               state->command->exit_code, state->command->command, state->current_line);
    }
    if (state->fatal_error) {
        return DESTROY_STATE;
    }
    return RESET_STATE;
}

int destroy_state(const struct dc_env *env, struct dc_error *err, void *arg) {
    struct state *state = arg;
    free(state);
    return DC_FSM_EXIT;
}


