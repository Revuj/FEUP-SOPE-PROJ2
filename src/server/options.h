#ifndef OPTIONS_H___
#define OPTIONS_H___

#define HELP_FLAG 'h'
#define HELP_LFLAG "help"
extern int o_show_help;

/** @name option_t */
/**@{ struct that holds the informations about the input options
 */
typedef struct {
    int bankOfficesNo; /**< @brief number of bank offices to be created*/
    char *password; /**< @brief administrator password*/
} option_t; /** @} end of the option_t struct */

/**
 * @brief Initializes the options
 * 
 * @return pointer to the options initialized
 */
option_t* init_options();

/**
 * @brief Parses the command lines arguments
 * 
 * @param argc - number of arguments on the command line
 * @param argv - arguments on the command line
 * @param options - options to be saved
 * @return 0 on success
 */
int parse_args(int argc, char** argv,option_t *options);

#endif // OPTIONS_H___