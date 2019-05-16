#ifndef OPTIONS_H___
#define OPTIONS_H___

#include <stdint.h>
#include "../types.h"

#define HELP_FLAG 'h'
#define HELP_LFLAG "help"
extern int o_show_help;

typedef struct {
    uint32_t account_id;
    char *password;
    uint32_t op_delay_ms;
    enum op_type type;
    char *operation_arguments;
} option_t;

option_t* init_options();

void free_options(option_t *options);

int parse_args(int argc, char** argv,option_t *option);

#endif // OPTIONS_H___