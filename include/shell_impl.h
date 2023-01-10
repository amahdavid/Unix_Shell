#ifndef DC_SHELL_SHELL_IMPL_H
#define DC_SHELL_SHELL_IMPL_H

#include <dc_env/env.h>
#include <dc_error/error.h>

int init_state(const struct dc_env *env, struct dc_error *err, void *arg);

int destroy_state(const struct dc_env *env, struct dc_error *err, void *arg);

int reset_state(const struct dc_env *env, struct dc_error *error, void *arg);

int read_commands(const struct dc_env *env, struct dc_error *err, void *arg);

int separate_commands(const struct dc_env *env, struct dc_error *err, void *arg);

int parse_commands(const struct dc_env *env, struct dc_error *err, void *arg);

int execute_commands(const struct dc_env *env, struct dc_error *err, void *arg);

int do_exit(const struct dc_env *env, struct dc_error *err, void *arg);

int handle_error(const struct dc_env *env, struct dc_error *err, void *arg);

#endif //DC_SHELL_SHELL_IMPL_H
