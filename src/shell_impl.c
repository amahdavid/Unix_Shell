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
#include <dc_posix/dc_stdio.h>
#include <dc_util/strings.h>
#include <dc_util/filesystem.h>

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
    state->command->line = NULL;
    state->command = NULL;

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
    struct state *state = (struct state *) arg;
    state->fatal_error = 0;
    size_t line_len = 0;

    char *cur_dir = dc_get_working_dir(env, err);
    if (dc_error_has_error(err)) {
        state->fatal_error = true;
        return ERROR;
    }

    fprintf(stdout, "[%s] %s", cur_dir, state->prompt);

    state->current_line = malloc(sizeof(char));
    if (dc_error_has_error(err)) {
        state->fatal_error = true;
    }

    dc_getline(env, err, &state->current_line, &line_len, stdin);
    if (dc_error_has_error(err)) {
        state->fatal_error = true;
        return ERROR;
    }

    dc_str_trim(env, state->current_line);
    line_len = strlen(state->current_line);

    if (line_len == 0) {
        return RESET_STATE;
    }

    state->current_line_length = line_len;

    return SEPARATE_COMMANDS;
}

int separate_commands(const struct dc_env *env, struct dc_error *err, void *arg) {

    struct state *state = arg;

    if (dc_error_has_error(err)) {
        state->fatal_error = true;
        return ERROR;
    }

    state->command = calloc(1, sizeof(command));

    if (state->command == NULL) {
        return ERROR;
    }

    state->command->line = strdup(state->current_line);

    if (state->command->line == NULL) {
        free(state->command);
        return ERROR;
    }

    state->command->command = NULL;
    state->command->exit_code = 0;
    state->command->argv = NULL;
    state->command->argc = 0;
    state->command->stdin_file = NULL;
    state->command->stdout_file = NULL;
    state->command->stdout_overwrite = false;
    state->command->stderr_file = NULL;

    return PARSE_COMMANDS;
}

int parse_commands(const struct dc_env *env, struct dc_error *err, void *arg) {
    struct state *state = arg;
    state->fatal_error = 0;
    parse_command(env, err, state, state->command);
    if (dc_error_has_error(err)) {
        state->fatal_error = 1;
        return ERROR;
    }
    return EXECUTE_COMMANDS;
}

int execute_commands(const struct dc_env *env,
                     struct dc_error *err, void *arg) {
    struct state *state = (struct state *) arg;
    if (strcmp(state->command->command, "cd") == 0) {
        builtin_cd(env, err, (struct command *) state->command->command);
    } else if (strcmp(state->command->command, "exit") == 0) {
        return DC_FSM_EXIT;
    } else {
        /*(struct command *) state->command->command,*/
        execute(env, err, state, state->path);
        if (dc_error_has_error(err)) {
            state->fatal_error = true;
        }
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

int handle_run_error(const struct dc_env *env, void *arg) {
    switch (errno) {
        case EACCES:
            printf("EACCES: %s: %s\n", (char *) arg, strerror(errno));
            break;

        case ELOOP:
            printf("ELOOP: %s: %s\n", (char *) arg, strerror(errno));
            break;

        case ENAMETOOLONG:
            printf("ENAMETOOLONG: %s: %s\n", (char *) arg, strerror(errno));
            break;

        case ENOENT:
            printf("ENOTDIR: %s: does not exist\n", (char *) arg);
            break;

        case ENOTDIR:
            printf("ENOTDIR: %s: is not a directory\n", (char *) arg);
            break;

        case E2BIG:
            printf("E2BIG: %s: \n", (char *) arg);
            break;

        case EINVAL:
            printf("EINVAL: %s: \n", (char *) arg);
            break;

        case ENOEXEC:
            printf("ENOEXEC: %s: \n", (char *) arg);
            break;

        case ENOMEM:
            printf("ENOMEM: %s: \n", (char *) arg);
            break;

        case ETXTBSY:
            printf("ETXTBSY: %s: \n", (char *) arg);
            break;

        default:
            printf("SHOULD NOT FUCKING GET HERE IDIOT\n");
            break;
    }
}

int destroy_state(const struct dc_env *env, struct dc_error *err, void *arg) {
    struct state *state = arg;
//    free(state);
    return DC_FSM_EXIT;
}


