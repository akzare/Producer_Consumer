/**
 * @file   main.c
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   06 August 2020
 * @version 0.1
 * @brief   main.c provides the main function for the Inter SoC Communication (ISC) 
 *          program. Its responsibility is to parse command-line options, detect and report 
 *          command-line errors, and configure and run the ISC.
 */

#include <errno.h>
#include <assert.h>
#include <getopt.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include "isc.h"


/*********************************************************************************** 
 * C o n s t a n t s ,   v a r i a b l e s ,  f u n c t i o n s 
************************************************************************************/

// The file to which to append the log string.
const char* main_log_filename = "isc.log";
FILE *main_log_fd = NULL;

// Description of long options for getopt_long.
static const struct option long_options[] = {
  { "help", 0, NULL, 'h' },
  { "protocol", 0, NULL, 'l' },
  { "addr", 0, NULL, 'a' },
  { "port", 0, NULL, 'p' },
  { "client", 0, NULL, 'c' },
  { "module-dir", 1, NULL, 'm' },
  { "verbose", 0, NULL, 'v' },
};

// Description of short options for getopt_long.
static const char* const short_options = "h:l:a:p:c:m:v";

// Usage summary text.
static const char* const usage_template =
  "Usage: %s [ options ]\n"
  " -h, --help Print this information.\n"
  " -l, --protocol network protocol.\n"
  " (by default, use tcp.\n"
  " -a, --addr host IP address.\n"
  " (by default, use local host 127.0.0.1).\n"
  " -p, --port port number.\n"
  " (by default, use 8080).\n"
  " -c, --client configure the system as client\n"
  " (by default, use executable directory).\n"
  " -m, --module-dir DIR Load modules from specified directory\n"
  " (by default, use executable directory).\n"
  " -v, --verbose Print verbose messages.\n";

// Print usage information and exit. If IS_ERROR is nonzero, write to
// stderr and use an error exit code. Otherwise, write to stdout and
// use a non-error termination code. Does not return.
static void print_usage (int is_error)
{
  fprintf (is_error ? stderr : stdout, usage_template, program_name);
  exit (is_error ? 1 : 0);
}

int main (int argc, char* const argv[])
{
  int next_option;

  // Store the program name, which we'll use in error messages.
  program_name = argv[0];

  // Don't print verbose messages.
  verbose = 0;

  // by defualt the system is server
  int is_client = 0;

  // The network protocol
  char* net_prtcl = "tcp";

  // The destination server IP address
  char* dest_ip_addr = SERVER_IP_ADDR;

  // The destination server port number
  int dest_port = SERVER_PORT;

  // Open the main log file for writing. If it exists, append to it;
  // otherwise, create a new file.
  main_log_fd = fopen (main_log_filename, "w");
  if (main_log_fd == NULL) {
    fprintf (stderr, "error: (%s) %s\n", "main_log_fd", strerror (errno));
  }

  // Load modules from the directory containing this executable.
  module_dir = get_self_executable_directory ();
  assert (module_dir != NULL);

  // Parse options.
  do {
    next_option =
       getopt_long (argc, argv, short_options, long_options, NULL);

    switch (next_option) {
      case 'h':
        // User specified -h or --help.
        print_usage(0);

      case 'l':
        // User specified -l or --protocol.
        {
          // use it.
          net_prtcl = strdup(optarg);
        }
        break;

      case 'a':
        // User specified -a or --addr.
        {
          // use it.
          dest_ip_addr = strdup(optarg);
        }
        break;

      case 'p':
        // User specified -p or --port.
        {
          // use it.
          dest_port = optarg;
        }
        break;

      case 'c':
        // User specified -c or --client.
        is_client = 1;
        break;

      case 'm':
        // User specified -m or --module-dir.
        {
          struct stat dir_info;

          // Check that it exists.
          if (access (optarg, F_OK) != 0)
            error (optarg, "module directory does not exist");

          // Check that it is accessible.
          if (access (optarg, R_OK | X_OK) != 0)
            error (optarg, "module directory is not accessible");

          // Make sure that it is a directory.
          if (stat (optarg, &dir_info) != 0 || !S_ISDIR (dir_info.st_mode))
            error (optarg, "not a directory");

          // It looks OK, so use it.
          module_dir = strdup(optarg);
        }
        break;

      case 'v':
        // User specified -v or --verbose.
        verbose = 1;
        break;

      case '?':
        // User specified an unrecognized option.
        print_usage(1);
      
      case -1:
        // Done with options.
        break;

      default:
        abort();
    }
  } while (next_option != -1);

  // This program takes no additional arguments. Issue an error if the
  // user specified any.
  if (optind != argc)
    print_usage (1);

  // Print the module directory, if we're running verbose.
  if (verbose) {
    printf ("modules will be loaded from %s\n", module_dir);
  }
  fprintf (main_log_fd, "\n%s - INFO - main - modules will be loaded from %s.", get_timestamp(), module_dir);

  // Run the isc.
  isc_run (net_prtcl, dest_ip_addr, dest_port, is_client);

  return 0;
}
