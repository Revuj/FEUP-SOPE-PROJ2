#ifndef OPTIONS_H___
#define OPTIONS_H___

#define HELP_FLAG 'h'
#define HELP_LFLAG "help"
extern int o_show_help;

#define USAGE_FLAG 
#define USAGE_LFLAG "usage"
extern int o_show_usage;

extern int id;
extern const char* password;
extern int delay;
extern int operation;
extern char* arguments;


// Parse command line arguments
int parse_args(int argc, char** argv);

#endif // OPTIONS_H___