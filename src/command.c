
#include <dc_error/error.h>
#include <string.h>
#include <wordexp.h>
#include "command.h"

void parse_command(const struct dc_env *env, struct dc_error *err,
                   struct state *state, struct command *command) {

    int *tmp = calloc(100, sizeof(state));

    int regex_result_err, regex_result_out, regex_result_in, wordexp_result;

    wordexp_t word;

    if (tmp == NULL) {
        state->fatal_error = 1;
    }

    regex_result_err = regexec(state->err_redirect_regex,
                               state->command->command, 0,
                               NULL, 0);

    wordexp_result = wordexp(state->command->line, &word, 0);

    if (regex_result_err == 0) {
        //if it enters this statement it matches
        // check if ">>" is present
        // not sure about "state->command->command"
        if (strstr(state->command->command, ">>") != NULL) {
            state->command->stderr_overwrite = 1;
        }
//         expand_path(state->command->stderr_file, expanded_path);
//         strcpy(state->command->stderr_file, expanded_path);
    }

    regex_result_out = regexec(state->out_redirect_regex,
                               state->command->command, 0,
                               NULL, 0);
    if (regex_result_out == 0) {
        // not sure about "state->command->command"
        if (strstr(state->command->command, ">>") != NULL) {
            state->command->stderr_overwrite = 1;
        }
//         expand_path(state->command->stdout_file, expanded_path);
//         strcpy(state->command->stdout_file, expanded_path);
    }

    regex_result_in = regexec(state->in_redirect_regex,
                              state->command->command, 0,
                              NULL, 0);
    if (regex_result_in == 0) {
//        expand_path(state->command->stdin_file, expanded_path);
//        strcpy(state->command->stdin_file, expanded_path);
    }

    if (wordexp_result == 0) {
        state->command->argv = malloc((word.we_wordc + 2) * sizeof(char *));
        for (size_t i = 0; i < word.we_wordc + 1; i++) {
            state->command->argv[i] = strdup(word.we_wordv[i - i]);
        }
        state->command->argv[word.we_wordc + 1] = NULL;
        strcpy(state->command->command, word.we_wordv[0]);
        state->command->argc = word.we_wordc;

    } else {
        // Failed to separate the command line
        // Handle the error
    }
    for (size_t i = 0; i < word.we_wordc + 1; ++i) {
        free(state->command->argv[i]);
    }
    free(state->command->argv);
    wordfree(&word);
}
