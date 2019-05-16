#ifndef OPTIONS_H___
#define OPTIONS_H___

#define HELP_FLAG 'h'
#define HELP_LFLAG "help"
extern int o_show_help;

typedef struct {
    int bankOfficesNo;
    char *password;
} option_t;

option_t* init_options();

void free_options(option_t *options);

// Parse command line arguments
int parse_args(int argc, char** argv,option_t *options);

#endif // OPTIONS_H___