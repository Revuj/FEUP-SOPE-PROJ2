#include "options.h"
#include "../constants.h"
#include "../types.h"

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

int o_show_help = false; // h, help

option_t* init_options() {
    option_t* options = (option_t *)malloc(sizeof(option_t));
    return options;
}

static void free_options(int status, void *args) {
    option_t *options = (option_t *)args;
    free((char*) options->password);
    free((char*) options->operation_arguments);
    free(options);
}

static const struct option long_options[] = {
    {HELP_LFLAG,              no_argument, &o_show_help,       true},
    {0, 0, 0, 0}
};

static const char* short_options = "+h";

static const wchar_t* usage = L"Usage: user [option] ID password delay operation_code arguments_list\n"
    "arguments_list is a space-separated string\n"
    "General:\n"
    "  -h, --help,           \n";

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

    if (endp == str || errno == ERANGE || result >= INT_MAX || result < 0) {
        return -1;
    } else {
        *store = (int)result;
        return 0;
    }
}

static void validateArgs(option_t *options) {
    if(options->account_id > MAX_BANK_ACCOUNTS){
        fprintf(stderr,"Invalid account id\n");
        exit(EXIT_SUCCESS);
    }

    size_t size = strlen(options->password);
    if(size < MIN_PASSWORD_LEN || size > MAX_PASSWORD_LEN) {
        fprintf(stderr,"Invalid password size\n");
        exit(EXIT_SUCCESS);
    }

    if(options->op_delay_ms > MAX_OP_DELAY_MS) {
        fprintf(stderr,"Invalid operation delay\n");
        exit(EXIT_SUCCESS);
    }
}

/**
 * Standard unix main's argument parsing function.
 */
int parse_args(int argc, char** argv,option_t *options) {

    if (argc == 1) {
        print_usage();
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
        if (parse_int(argv[optind++], (int*)&(options->account_id)) != 0) {
            print_badpositional(1);
        }
        options->password = strdup(argv[optind++]);
        if (parse_int(argv[optind++], (int*)&(options->op_delay_ms)) != 0) {
            print_badpositional(3);
        }
        if (parse_int(argv[optind++], (int*)&(options->type)) != 0) {
            print_badpositional(4);
        }
        options->operation_arguments = strdup(argv[optind++]);
    } else {
        print_numpositional(num_positional);
    }

    validateArgs(options);

    on_exit(free_options,options);

    return 0;
}