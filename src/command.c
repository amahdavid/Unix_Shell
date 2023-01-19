#include "command.h"
#include "shell.h"
#include "shell_impl.h"
#include "util.h"
#include <dc_posix/dc_unistd.h>

int parse_command(const struct dc_env *env, struct dc_error *err,
                  struct state *state, struct command *command) {

    if (dc_error_has_error(err)) {
        state->fatal_error = true;
        return ERROR;
    }

    int regex_result_err, regex_result_out, regex_result_in, wordexp_result;
    wordexp_t word;

    regex_result_err = regexec(state->err_redirect_regex,
                               state->command->line, 1,
                               NULL, 0);
    if (regex_result_err == 0) {

        regmatch_t reg_match;
        regexec(state->err_redirect_regex, state->command->line, 1, &reg_match, 0);
        size_t redirect_len = reg_match.rm_eo - reg_match.rm_so;
        char *redirect = strndup(state->command->line + reg_match.rm_so, redirect_len);

        if (redirect == NULL) {
            state->fatal_error = true;
            return ERROR;
        }
        if (strstr(redirect, ">>")) {
            state->command->stderr_overwrite = true;
        }

        state->command->stderr_file = expand_path(env, err, redirect);

        if (state->command->stderr_file == NULL) {
            state->fatal_error = true;
            free(redirect);
            return ERROR;
        }
        free(redirect);
        state->command->line[reg_match.rm_so] = '\0';
    }

    regex_result_out = regexec(state->out_redirect_regex,
                               state->command->line, 1,
                               NULL, 0);
    if (regex_result_out == 0) {

        regmatch_t reg_match;
        regexec(state->out_redirect_regex, state->command->line, 1, &reg_match, 0);
        size_t redirect_len = reg_match.rm_eo - reg_match.rm_so;
        char *redirect = strndup(state->command->line + reg_match.rm_so, redirect_len);

        if (redirect == NULL) {
            state->fatal_error = true;
            return ERROR;
        }
        if (strstr(redirect, ">>")) {
            state->command->stdout_overwrite = true;
        }

        state->command->stdout_file = expand_path(env, err, redirect);

        if (state->command->stdout_file == NULL) {
            state->fatal_error = true;
            free(redirect);
            return ERROR;
        }

        free(redirect);
        state->command->line[reg_match.rm_so] = '\0';
    }

    regex_result_in = regexec(state->in_redirect_regex,
                              state->command->line, 1,
                              NULL, 0);
    if (regex_result_in == 0) {

        regmatch_t reg_match;
        regexec(state->in_redirect_regex, state->command->line, 1, &reg_match, 0);
        size_t redirect_len = reg_match.rm_eo - reg_match.rm_so;
        char *redirect = strndup(state->command->line + reg_match.rm_so, redirect_len);

        if (redirect == NULL) {
            state->fatal_error = true;
            return ERROR;
        }

        state->command->stdin_file = expand_path(env, err, redirect);

        if (state->command->stdin_file == NULL) {
            state->fatal_error = true;
            free(redirect);
            return ERROR;
        }

        free(redirect);
        state->command->line[reg_match.rm_so] = '\0';
    }

    wordexp_result = dc_wordexp(env, err, state->command->line, &word, 0);
    if (wordexp_result == 0) {

        state->command->argc = word.we_wordc;
        state->command->argv = (char **) calloc(1, (word.we_wordc + 2) * sizeof(char *));
        state->command->command = (char *) calloc(1, (strlen(word.we_wordv[0]) + 1));

        for (size_t i = 1; i < word.we_wordc + 1; ++i) {
            state->command->argv[i] = strdup(word.we_wordv[i - 1]);
        }

        state->command->argv[word.we_wordc + 1] = NULL;
        state->command->command = strdup(word.we_wordv[0]);
        wordfree(&word);

    } else {
        printf("File or directory does not exist: %s\n", state->command->line);
        handle_error(env, err, state);
    }
    return EXECUTE_COMMANDS;
}

int redirect(const struct dc_env *env, struct dc_error *err, void *arg) {

    struct state *state = (struct state *) arg;
    FILE *file;
    if (dc_error_has_error(err)) {
        fclose(file);
        return ERROR;
    }

    if (state->command->stdin_file != NULL) {
        file = fopen(state->command->stdin_file, O_RDONLY);
        if (file == NULL) {
            if (state->command->stdout_file != NULL) {
                fclose(file);
            }
            if (state->command->stderr_file != NULL) {
                fclose(file);
            }
            return ERROR;
        }
        dc_dup2(env, err, fileno(file), STDIN_FILENO);
    }

    if (state->command->stdout_file != NULL) {
        if (state->command->stderr_overwrite) {
            file = fopen(state->command->stdout_file, "w");
        } else {
            file = fopen(state->command->stdout_file, "a");
        }
        if (file == NULL) {
            if (state->command->stderr_file != NULL) {
                fclose(file);
            }
            return ERROR;
        }
        dc_dup2(env, err, fileno(file), STDOUT_FILENO);
    }

    if (state->command->stderr_file != NULL) {
        if (state->command->stderr_overwrite) {
            file = fopen(state->command->stderr_file, "w");
        } else {
            file = fopen(state->command->stderr_file, "a");
        }
        if (file == NULL) {
            handle_error(env, err, state->command);
            printf("redirect in command.c stderr_file");
            return ERROR;
        }
        dc_dup2(env, err, fileno(file), STDERR_FILENO);
    }
}

