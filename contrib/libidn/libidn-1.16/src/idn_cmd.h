/** @file idn_cmd.h
 *  @brief The header file for the command line option parser
 *  generated by GNU Gengetopt version 2.22.2
 *  http://www.gnu.org/software/gengetopt.
 *  DO NOT modify this file, since it can be overwritten
 *  @author GNU Gengetopt by Lorenzo Bettini */

#ifndef IDN_CMD_H
#define IDN_CMD_H

/* If we use autoconf.  */
#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdio.h> /* for FILE */

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifndef CMDLINE_PARSER_PACKAGE
/** @brief the program name (used for printing errors) */
#define CMDLINE_PARSER_PACKAGE "idn"
#endif

#ifndef CMDLINE_PARSER_PACKAGE_NAME
/** @brief the complete program name (used for help and version) */
#define CMDLINE_PARSER_PACKAGE_NAME "idn"
#endif

#ifndef CMDLINE_PARSER_VERSION
/** @brief the program version */
#define CMDLINE_PARSER_VERSION VERSION
#endif

/** @brief Where the command line options are stored */
struct gengetopt_args_info
{
  const char *help_help; /**< @brief Print help and exit help description.  */
  const char *full_help_help; /**< @brief Print help, including hidden options, and exit help description.  */
  const char *version_help; /**< @brief Print version and exit help description.  */
  const char *stringprep_help; /**< @brief Prepare string according to nameprep profile help description.  */
  const char *punycode_decode_help; /**< @brief Decode Punycode help description.  */
  const char *punycode_encode_help; /**< @brief Encode Punycode help description.  */
  const char *idna_to_ascii_help; /**< @brief Convert to ACE according to IDNA (default) help description.  */
  const char *idna_to_unicode_help; /**< @brief Convert from ACE according to IDNA help description.  */
  int allow_unassigned_flag;	/**< @brief Toggle IDNA AllowUnassigned flag (default=off).  */
  const char *allow_unassigned_help; /**< @brief Toggle IDNA AllowUnassigned flag help description.  */
  int usestd3asciirules_flag;	/**< @brief Toggle IDNA UseSTD3ASCIIRules flag (default=off).  */
  const char *usestd3asciirules_help; /**< @brief Toggle IDNA UseSTD3ASCIIRules flag help description.  */
  int tld_flag;	/**< @brief Check string for TLD specific rules\nOnly for --idna-to-ascii and --idna-to-unicode (default=on).  */
  const char *tld_help; /**< @brief Check string for TLD specific rules\nOnly for --idna-to-ascii and --idna-to-unicode help description.  */
  int no_tld_flag;	/**< @brief Don't check string for TLD specific rules\nOnly for --idna-to-ascii and --idna-to-unicode (default=off).  */
  const char *no_tld_help; /**< @brief Don't check string for TLD specific rules\nOnly for --idna-to-ascii and --idna-to-unicode help description.  */
  const char *nfkc_help; /**< @brief Normalize string according to Unicode v3.2 NFKC help description.  */
  const char *hidden_nfkc_help; /**< @brief Hidden parameter for backwards compatibility help description.  */
  char * profile_arg;	/**< @brief Use specified stringprep profile instead\nValid stringprep profiles are `Nameprep', `iSCSI', `Nodeprep', `Resourceprep', `trace', and `SASLprep'..  */
  char * profile_orig;	/**< @brief Use specified stringprep profile instead\nValid stringprep profiles are `Nameprep', `iSCSI', `Nodeprep', `Resourceprep', `trace', and `SASLprep'. original value given at command line.  */
  const char *profile_help; /**< @brief Use specified stringprep profile instead\nValid stringprep profiles are `Nameprep', `iSCSI', `Nodeprep', `Resourceprep', `trace', and `SASLprep'. help description.  */
  int debug_flag;	/**< @brief Print debugging information (default=off).  */
  const char *debug_help; /**< @brief Print debugging information help description.  */
  int quiet_flag;	/**< @brief Silent operation (default=off).  */
  const char *quiet_help; /**< @brief Silent operation help description.  */
  
  unsigned int help_given ;	/**< @brief Whether help was given.  */
  unsigned int full_help_given ;	/**< @brief Whether full-help was given.  */
  unsigned int version_given ;	/**< @brief Whether version was given.  */
  unsigned int stringprep_given ;	/**< @brief Whether stringprep was given.  */
  unsigned int punycode_decode_given ;	/**< @brief Whether punycode-decode was given.  */
  unsigned int punycode_encode_given ;	/**< @brief Whether punycode-encode was given.  */
  unsigned int idna_to_ascii_given ;	/**< @brief Whether idna-to-ascii was given.  */
  unsigned int idna_to_unicode_given ;	/**< @brief Whether idna-to-unicode was given.  */
  unsigned int allow_unassigned_given ;	/**< @brief Whether allow-unassigned was given.  */
  unsigned int usestd3asciirules_given ;	/**< @brief Whether usestd3asciirules was given.  */
  unsigned int tld_given ;	/**< @brief Whether tld was given.  */
  unsigned int no_tld_given ;	/**< @brief Whether no-tld was given.  */
  unsigned int nfkc_given ;	/**< @brief Whether nfkc was given.  */
  unsigned int hidden_nfkc_given ;	/**< @brief Whether hidden-nfkc was given.  */
  unsigned int profile_given ;	/**< @brief Whether profile was given.  */
  unsigned int debug_given ;	/**< @brief Whether debug was given.  */
  unsigned int quiet_given ;	/**< @brief Whether quiet was given.  */

  char **inputs ; /**< @brief unamed options (options without names) */
  unsigned inputs_num ; /**< @brief unamed options number */
} ;

/** @brief The additional parameters to pass to parser functions */
struct cmdline_parser_params
{
  int override; /**< @brief whether to override possibly already present options (default 0) */
  int initialize; /**< @brief whether to initialize the option structure gengetopt_args_info (default 1) */
  int check_required; /**< @brief whether to check that all required options were provided (default 1) */
  int check_ambiguity; /**< @brief whether to check for options already specified in the option structure gengetopt_args_info (default 0) */
  int print_errors; /**< @brief whether getopt_long should print an error message for a bad option (default 1) */
} ;

/** @brief the purpose string of the program */
extern const char *gengetopt_args_info_purpose;
/** @brief the usage string of the program */
extern const char *gengetopt_args_info_usage;
/** @brief all the lines making the help output */
extern const char *gengetopt_args_info_help[];
/** @brief all the lines making the full help output (including hidden options) */
extern const char *gengetopt_args_info_full_help[];

/**
 * The command line parser
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser (int argc, char * const *argv,
  struct gengetopt_args_info *args_info);

/**
 * The command line parser (version with additional parameters - deprecated)
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @param override whether to override possibly already present options
 * @param initialize whether to initialize the option structure my_args_info
 * @param check_required whether to check that all required options were provided
 * @return 0 if everything went fine, NON 0 if an error took place
 * @deprecated use cmdline_parser_ext() instead
 */
int cmdline_parser2 (int argc, char * const *argv,
  struct gengetopt_args_info *args_info,
  int override, int initialize, int check_required);

/**
 * The command line parser (version with additional parameters)
 * @param argc the number of command line options
 * @param argv the command line options
 * @param args_info the structure where option information will be stored
 * @param params additional parameters for the parser
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_ext (int argc, char * const *argv,
  struct gengetopt_args_info *args_info,
  struct cmdline_parser_params *params);

/**
 * Save the contents of the option struct into an already open FILE stream.
 * @param outfile the stream where to dump options
 * @param args_info the option struct to dump
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_dump(FILE *outfile,
  struct gengetopt_args_info *args_info);

/**
 * Save the contents of the option struct into a (text) file.
 * This file can be read by the config file parser (if generated by gengetopt)
 * @param filename the file where to save
 * @param args_info the option struct to save
 * @return 0 if everything went fine, NON 0 if an error took place
 */
int cmdline_parser_file_save(const char *filename,
  struct gengetopt_args_info *args_info);

/**
 * Print the help
 */
void cmdline_parser_print_help(void);
/**
 * Print the full help (including hidden options)
 */
void cmdline_parser_print_full_help(void);
/**
 * Print the version
 */
void cmdline_parser_print_version(void);

/**
 * Initializes all the fields a cmdline_parser_params structure 
 * to their default values
 * @param params the structure to initialize
 */
void cmdline_parser_params_init(struct cmdline_parser_params *params);

/**
 * Allocates dynamically a cmdline_parser_params structure and initializes
 * all its fields to their default values
 * @return the created and initialized cmdline_parser_params structure
 */
struct cmdline_parser_params *cmdline_parser_params_create(void);

/**
 * Initializes the passed gengetopt_args_info structure's fields
 * (also set default values for options that have a default)
 * @param args_info the structure to initialize
 */
void cmdline_parser_init (struct gengetopt_args_info *args_info);
/**
 * Deallocates the string fields of the gengetopt_args_info structure
 * (but does not deallocate the structure itself)
 * @param args_info the structure to deallocate
 */
void cmdline_parser_free (struct gengetopt_args_info *args_info);

/**
 * Checks that all the required options were specified
 * @param args_info the structure to check
 * @param prog_name the name of the program that will be used to print
 *   possible errors
 * @return
 */
int cmdline_parser_required (struct gengetopt_args_info *args_info,
  const char *prog_name);


#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif /* IDN_CMD_H */
