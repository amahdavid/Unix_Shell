#include <string.h>
#include "state.h"
#include <dc_env/env.h>
#include <dc_error/error.h>
#include <malloc.h>

void do_reset_state(const struct dc_env *env,
                    struct dc_error *err, struct state *state) {
    free(state->current_line);
    state->current_line = NULL;
    memset(err, 0, sizeof(state));
    // return read commands (not done)
}
