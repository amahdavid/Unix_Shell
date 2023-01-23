#include "command.h"
#include "shell.h"
#include "shell_impl.h"
#include "util.h"
#include <dc_posix/dc_unistd.h>

int parse_command(const struct dc_env *env, struct dc_error *err,
                  struct state *state) {

    if (dc_error_has_error(err)) {
        state->fatal_error = true;
        return ERROR;
    }
    // THE REGEX RETURNS 0 IF IT MATCHES AND 1 IF IT DOES NOT
    regmatch_t match;
    int regex_result_err, regex_result_out, regex_result_in, wordexp_result;
    wordexp_t exp;

    regex_result_err = regexec(state->err_redirect_regex,
                               state->command->line, 1,
                               &match, 0);
    if (regex_result_err == 0) {

        size_t redirect_len = match.rm_eo - match.rm_so;
        char *redirect = strndup(state->command->line + match.rm_so, redirect_len);

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
        state->command->line[match.rm_so] = '\0';
    }

    regex_result_out = regexec(state->out_redirect_regex,
                               state->command->line, 1,
                               &match, 0);
    if (regex_result_out == 0) {
        size_t redirect_len = match.rm_eo - match.rm_so;
        char *redirect = strndup(state->command->line + match.rm_so, redirect_len);

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
        state->command->line[match.rm_so] = '\0';
    }

    regex_result_in = regexec(state->in_redirect_regex,
                              state->command->line, 1,
                              &match, 0);
    if (regex_result_in == 0) {
        size_t redirect_len = match.rm_eo - match.rm_so;
        char *redirect = strndup(state->command->line + match.rm_so, redirect_len);

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
        state->command->line[match.rm_so] = '\0';
    }

    wordexp_result = dc_wordexp(env, err, state->command->line, &exp, 0);
    if (wordexp_result == 0) {

        state->command->argc = exp.we_wordc;
        state->command->argv = (char **) calloc(1, (exp.we_wordc + 2) * sizeof(char *));

        for (size_t i = 0; i < exp.we_wordc; ++i) {
            state->command->argv[i] = strdup(exp.we_wordv[i]);
        }

        state->command->argv[exp.we_wordc] = NULL;
        state->command->command = strdup(exp.we_wordv[0]);
        wordfree(&exp);

    } else {
        printf("unable to parse: %s\n", state->command->line);
//        handle_error(env, err, state->command->command);
    }
    return EXECUTE_COMMANDS;
}

void redirect(const struct dc_env *env, struct dc_error *err, void *arg) {

    struct state *state = (struct state *) arg;
    int fd;

    if (dc_error_has_error(err)) {
        handle_error(env, err, state);
        return;
    }

    if (state->command->stdin_file != NULL) {
        // THIS OPENS THE NEW STDIN FILE
        fd = open(state->command->stdin_file, O_RDONLY, S_IRWXO | S_IRWXG | S_IRWXU);
        // CHANGES THE STANDARD IN FILE TO NEW STDIN_FILE
        dc_dup2(env, err, fd, STDIN_FILENO);
        if (dc_error_has_error(err)) {
            state->fatal_error = true;
            return;
        }
        // CLOSES THE FILE
        dc_close(env, err, fd);
    }

    if (state->command->stdout_file != NULL) {
        // CHECKS IF IT SHOULD OVERWRITE OR TRUNCATE
        if (state->command->stderr_overwrite == true) {
            // OPENS THE STDOUT_FILE TO TRUNCATE
            fd = open(state->command->stdout_file,
                      O_CREAT | O_RDWR | O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
        } else {
            // OPENS THE STDOUT_FILE TO APPEND
            fd = open(state->command->stdout_file,
                      O_CREAT | O_RDWR | O_APPEND, S_IRWXO | S_IRWXG | S_IRWXU);
        }
        // CHANGES THE STANDARD IN FILE TO NEW STDOUT_FILENO
        dc_dup2(env, err, fd, STDOUT_FILENO);
        if (dc_error_has_error(err)) {
            state->fatal_error = true;
            return;
        }
        // CLOSES THE FILE
        dc_close(env, err, fd);
    }

    if (state->command->stderr_file != NULL) {
        // CHECKS IF IT SHOULD OVERWRITE OR TRUNCATE
        if (state->command->stderr_overwrite == true) {
            fd = open(state->command->stderr_file,
                      O_CREAT | O_RDWR | O_TRUNC, S_IRWXO | S_IRWXG | S_IRWXU);
        } else {
            fd = open(state->command->stderr_file,
                      O_CREAT | O_RDWR | O_APPEND, S_IRWXO | S_IRWXG | S_IRWXU);
        }
        dc_dup2(env, err, fd, STDERR_FILENO);
        if (dc_error_has_error(err)) {
            state->fatal_error = true;
            return;
        }
        // CLOSES THE FILE
        dc_close(env, err, fd);
    }
}

