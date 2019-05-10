#include "options.h"
#include "../constants.h"

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <getopt.h>
#include <stdbool.h>
#include <locale.h>
#include <wchar.h>
#include <math.h>
#include <limits.h>

// <!--- OPTIONS (Resolve externals of options.h)
int o_show_help = false; // h, help
int o_show_usage = false; // usage

int id;
const char* password = NULL;
int delay;
int operation;
char* arguments = NULL;
// ----> END OF OPTIONS


static const struct option long_options[] = {
    // general options
    {HELP_LFLAG,              no_argument, &o_show_help,       true},
    {USAGE_LFLAG,             no_argument, &o_show_usage,      true},
    // end of options
    {0, 0, 0, 0}
};

// enforce POSIX with +
static const char* short_options = "+h";
// x for no_argument, x: for required_argument, x:: for optional_argument


static const wchar_t* usage = L"Usage: user [option] ID password delay operation_code arguments_list\n"
    "arguments_list is a space-separated string\n"
    "General:\n"
    "  -h, --help,           \n"
    "      --usage           Show this message and exit\n";

static void clear_options() {
    free((char*)password);
    free((char*)arguments);
}

static void print_all() {
    setlocale(LC_ALL, "");
    wprintf(usage);
    exit(EXIT_SUCCESS);
}

static void print_usage() {
    setlocale(LC_ALL, "");
    wprintf(usage);
    exit(EXIT_SUCCESS);
}

static void print_numpositional(int n) {
    setlocale(LC_ALL, "");
    wprintf(L"Error: Expected 5 positional arguments, but got %d.\n%S", n, usage);
    exit(EXIT_SUCCESS);
}

static void print_badpositional(int i) {
    setlocale(LC_ALL, "");
    wprintf(L"Error: Positional argument #%d is invalid.\n%S", i, usage);
    exit(EXIT_SUCCESS);
}

static int parse_int(const char* str, int* store) {
    char* endp;
    long result = strtol(str, &endp, 10);

    if (endp == str || errno == ERANGE || result >= INT_MAX || result <= INT_MIN) {
        return -1;
    } else {
        *store = (int)result;
        return 0;
    }
}

/**
 * Standard unix main's argument parsing function.
 */
int parse_args(int argc, char** argv) {
    // If there are no args, print usage message and exit
    if (argc == 1) {
        print_all();
    }

    // Standard getopt_long Options Loop
    while (true) {
        int c, lindex = 0;

        c = getopt_long(argc, argv, short_options,
            long_options, &lindex);

        if (c == -1) break; // No more options

        switch (c) {
        case HELP_FLAG:
            o_show_help = true;
            break;
        case '?':
        case ':':
        default:
            // getopt_long already printed an error message.
            print_usage();
        }
    } // End [Options Loop] while

    if (o_show_help) {
        print_usage();
    }

    // Exactly 5 positional arguments are expected
    int num_positional = argc - optind;

    if (num_positional == 5) {
        if (parse_int(argv[optind++], &id) != 0 || id > MAX_BANK_ACCOUNTS) {
            print_badpositional(1);
        }
        password = strdup(argv[optind++]);
        if (strlen(password) < MIN_PASSWORD_LEN || strlen(password) > MAX_PASSWORD_LEN) {
            print_badpositional(2);
        }
        if (parse_int(argv[optind++], &delay) != 0 || delay > MAX_OP_DELAY_MS) {
            print_badpositional(3);
        }
        if (parse_int(argv[optind++], &operation) != 0 || operation < 0 || operation > 3) {
            print_badpositional(4);
        }
        arguments = strdup(argv[optind++]);
    } else {
        print_numpositional(num_positional);
    }

    atexit(clear_options);
    return 0;
}