
#include <dc_error/error.h>
#include <string.h>
#include <wordexp.h>
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "command.h"
#include "shell.h"
#include "shell_impl.h"
#include "util.h"
#include <dc_posix/dc_wordexp.h>

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
        // INSERT NULL TERMINATOR BEFORE REDIRECT
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

    wordexp_result = wordexp(state->command->line, &word, 0);
    if (wordexp_result == 0) {
        if (word.we_wordc == 0) {
            handle_error(env, err, state);
        } else {
            char *redirect = malloc(strlen(word.we_wordv[0]));
            if (dc_error_has_error(err)) {
                state->fatal_error = true;
                return ERROR;
            }
            strcpy(redirect, word.we_wordv[0]);
        }
    } else {
        printf("File or directory does not exist: %s\n", state->command->line);
        printf("wordexp() failed: %s\n", strerror(wordexp_result));
        handle_error(env, err, state);
    }
    // this line causes a SIGSEGV ERROR
    wordfree(&word);
    return EXECUTE_COMMANDS;
}

void redirect(const struct dc_env *env, const struct dc_error *err, struct command *command) {
    if (dc_error_has_error(err)) {
        printf("function redirect in command.c 'close any open files and return'");
        return;
    }
    if (command->stdin_file != NULL) {
        int fd = open(command->stdin_file, O_RDONLY);
        if (fd == -1) {
            //handle_error(DC_FILE_OPEN_FAILED, err);
            // close files
            return;
        }
        dup2(fd, STDIN_FILENO);
        close(fd);
    }

    if (command->stdout_file != NULL) {
        int fd;
        if (command->stderr_overwrite) {
            fd = open(command->stdout_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        } else {
            fd = open(command->stdout_file, O_WRONLY | O_CREAT | O_APPEND | S_IRUSR | S_IWUSR);
        }
        if (fd == -1) {
            //handle_error(DC_FILE_OPEN_FAILED, err);
            printf("redirect in command.c stdout.file");
            //close file
            return;
        }
        dup2(fd, STDOUT_FILENO);
    }

    if (command->stderr_file != NULL) {
        int fd;
        if (command->stderr_overwrite) {
            fd = open(command->stderr_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
        } else {
            fd = open(command->stderr_file, O_WRONLY | O_CREAT | O_APPEND | S_IRUSR | S_IWUSR);
        }
        if (fd == -1) {
            //handle_error(DC_FILE_OPEN_FAILED, err);
            printf("redirect in command.c stderr_file");
            //close file
            return;
        }
        dup2(fd, STDERR_FILENO);
    }
}

