#include <stdio.h>
#include <cgreen/internal/suite_internal.h>
#include <cgreen/reporter.h>
#include <cgreen/suite.h>
#include <cgreen/text_reporter.h>
#include "tests.h"

int main(int argc, char **argv) {
    TestSuite *suite;
    TestReporter *reporter;
    int suite_result;

    suite = create_test_suite();
    reporter = create_text_reporter();

    add_suite(suite, builtin_tests());
    add_suite(suite, command_tests());
    add_suite(suite, execute_tests());
    add_suite(suite, input_tests());
    add_suite(suite, shell_impl_tests());
    add_suite(suite, util_tests());


    if (argc > 1){
        suite_result = run_single_test(suite, argv[1], reporter);
    } else
    {
        suite_result = run_test_suite(suite, reporter);
    }
}
