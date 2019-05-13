#ifndef OPTIONS_H___
#define OPTIONS_H___

#include <stdint.h>
#include "../types.h"

#define HELP_FLAG 'h'
#define HELP_LFLAG "help"
extern int o_show_help;

#define USAGE_FLAG 
#define USAGE_LFLAG "usage"
extern int o_show_usage;

// Parse command line arguments
int parse_args(int argc, char** argv,tlv_request_t *request);

#endif // OPTIONS_H___