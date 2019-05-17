#ifndef OPTIONS_H___
#define OPTIONS_H___

#include <stdint.h>
#include "../types.h"

#define HELP_FLAG 'h'
#define HELP_LFLAG "help"
extern int o_show_help;

/** @name option_t */
/**@{ struct that holds the informations about the input options
 */
typedef struct {
    uint32_t account_id; /**< @brief id of the accounts that wants to make the request*/
    char *password; /**< @brief password of the accounts*/
    uint32_t op_delay_ms; /**< @brief delay of the operation*/
    enum op_type type; /**< @brief type of operation*/
    char *operation_arguments; /**< @brief arguments of the operation*/
} option_t; /** @} end of the option_t struct */

/**
 * @brief Initializes the options
 * 
 * @return pointer to the options initialized
 */
option_t* init_options();

/**
 * @brief Frees space used to save options
 * 
 * @param options to be freed
 */
void free_options(option_t *options);

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