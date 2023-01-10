#include "tests.h"
#include "util.h"
#include <dc_env/env.h>
#include <dc_error/error.h>
#include <dc_posix/dc_stdlib.h>
#include "state.h"

Describe(util);

dc_env_tracer tracer;
static struct dc_env *env;
static struct dc_error *err;

BeforeEach(util){
    err = dc_error_create(false);
    env = dc_env_create(err, false, tracer);

    dc_error_init(err, false);
    dc_env_set_tracer(env, tracer);
}

AfterEach(util){}

Ensure(util, get_path){
    dc_setenv(&env, &err, "PS1", NULL, true);
    get_prompt(&env, &err);
}

TestSuite *util_tests(void){
    TestSuite *suite;
    suite = create_test_suite();
    add_test_with_context(suite, util, get_path);
}

char *state_to_string(const struct dc_posix_env *env, const struct state *state){
    size_t len;
    char *line;

    if(state->current_line == NULL){
        len = strlen("current line = NULL");
    }
    else
    {
        len = strlen("current line = \"\"");
        len += state->current_line_length;
    }

    len += strlen(", fatal error = ");
    line = malloc(len + 1 + 100);

    if (state->current_line == NULL){
        sprintf(line, "current_line = NULL, fatal error = %d", state->fatal_error);
    } else{
        sprintf(line, "current_line = \"%s\", fatal error = %d", state->current_line, state->fatal_error);
    }

    return line;
}

//void display_state(const struct dc_posix_env *env, const struct state *state, FILE *stream)
//{
//    char *str;
//    str = state_to_string(env, state);
//    fprintf(stream, "%s\n", str);
//    free(str);
//}