/*
 * @file   isc.h
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   01 August 2020
 * @version 0.1
 * @brief   For simplicity, all exported functions and variables are declared in
 *          a single header file, isc.h, which is included by the other files.
 *          Functions that are intended for use within a single compilation unit 
 *          only are declared static and are not declared in isc.h.
 */

#ifndef ISC_H
#define ISC_H

#include <netinet/in.h>
#include <sys/types.h>
#include <stdbool.h>
#include <pthread.h>

/*********************************************************************************** 
 * S y m b o l s   d e f i n e d   i n   c o m m o n . c . 
************************************************************************************/

#define SERVER_PORT 8080
#define SERVER_IP_ADDR "127.0.0.1"


/* The name of this program. The value of program_name is the name of the program being run, 
 * as specified in its argument list. When the program is invoked from the shell, this is 
 * the path and name of the program as the user entered it.
 */
extern const char* program_name;
extern const char* main_log_filename;
extern FILE *main_log_fd;


/* If nonzero, print verbose messages. The variable verbose is nonzero if the program is 
 * running in verbose mode. In this case, various parts of the program print progress 
 * messages to stdout.
*/
extern int verbose;


/* Like malloc, except aborts the program if allocation fails. 
 * xmalloc, xrealloc, and xstrdup are error-checking versions of the C library
 * functions malloc, realloc, and strdup, respectively. Unlike the standard versions,
 * which return a null pointer if the allocation fails, these functions immediately
 * abort the program when insufficient memory is available.
 */
extern void* xmalloc (size_t size);


/* Like realloc, except aborts the program if allocation fails. 
 */
extern void* xrealloc (void* ptr, size_t size);


/* Like strdup, except aborts the program if allocation fails. 
 */
extern char* xstrdup (const char* s);


/* Print an error message for a failed call OPERATION, using the value of errno, and 
 * end the program. 
 * The error function is for reporting a fatal program error. It prints a message to
 * stderr and ends the program. For errors caused by failed system calls or library
 * calls, system_error generates part of the error message from the value of errno.
 */
extern void system_error (const char* operation);


/* Print an error message for failure involving CAUSE, including a
 * descriptive MESSAGE, and end the program. 
 */
extern void error (const char* cause, const char* message);


/* Return the directory containing the running program's executable.
 * The return value is a memory buffer that the caller must deallocate
 * using free. This function calls abort on failure. 
 */
extern char* get_self_executable_directory ();


/* better_sleep provides an alternate implementation of sleep. Unlike
 * the ordinary system call, this function takes a floating-point value for the number of
 * seconds to sleep and restarts the sleep operation if it's interrupted by a signal. 
 */
int better_sleep (double sleep_time);

/* Obtain a binary semaphore's ID, allocating if necessary. 
 */
int binary_semaphore_allocation (key_t key, int sem_flags);

/* Deallocate a binary semaphore. All users must have finished their
 * use. Returns -1 on failure. 
 */
int binary_semaphore_deallocate (int semid);

/* Initialize a binary semaphore with a value of 1. 
 */
int binary_semaphore_initialize (int semid);

/* Wait on a binary semaphore. Block until the semaphore value is positive, then
 *  decrement it by 1. 
 */
int binary_semaphore_wait (int semid);

/* Post to a binary semaphore: increment its value by 1.
 * This returns immediately. 
 */
int binary_semaphore_post (int semid);


void print_time ();

/* Return a character string representing the current date and time. 
 */
char* get_timestamp ();


/*********************************************************************************** 
 * S y m b o l s   d e f i n e d   i n   m o d u l e . c 
 ***********************************************************************************/

/* An instance of a loaded server module. 
 */
struct ipc_module {
  /* The shared library handle corresponding to the loaded module. 
   */
  void* handle;
  /* A name describing the module. 
   */
  const char* name;
  /* The function is used to initialize the loadable IPC module.
   */
  void (* init_function) (void (*ipc_rec)(uint8_t *buf, int32_t bufSize), void (*ipc_xmit)(uint8_t *buf, int32_t bufSize));
  /* The function is used to clean up the loadable IPC module footage from memory.
   */
  void (* cleanup_function) ();
  /* The function is used to transmit data over a communication interface (socket, 
     shared memory, or PCIe) to another process.
   */
  uint32_t (* xmit_function) (uint8_t *buf, int32_t bufSize);
  /* The function is used to receive data from PCIe and and then
     copy data footage into IPC module (i.e. shared memory) for processing in this SoC.
   */
  void (* stop_function) ();
  bool (* start_function) ();
  bool (* wait4Done_function) ();
  bool (* set_param_function) (const char* prtcl, const char *addr, int port);
  void (* rec_function) (uint8_t *buf, int32_t bufSize);
};


/* The directory from which modules are loaded. */
extern char* module_dir;


/* Attempt to load an Inter Process(IPC) module with the name MODULE_PATH. If an
 * IPC module exists with this path, loads the module and returns a
 * ipc_module structure representing it. Otherwise, returns NULL. 
 */
extern struct ipc_module* ipc_open (const char* module_path);


/* Close the Inter Process Communication module and deallocate the MODULE object. 
 */
extern void ipc_close (struct ipc_module* module);


/* Make a 1K shared memory segment for producer */
#define PROD_SHM_SIZE 512
#define PROD_TEST_REGION_SIZE PROD_SHM_SIZE


/* Make a 1K shared memory segment for consumer */
#define CONS_SHM_SIZE 512 
#define CONS_TEST_REGION_SIZE CONS_SHM_SIZE


/*********************************************************************************** 
 * S y m b o l s   d e f i n e d   i n   i s c . c . 
***********************************************************************************/

/* Run the Inter SoC Communication kernel. 
 */
extern void isc_run (const char* net_prtcl, const char* dest_ip_addr, int dest_port, int is_client);

#endif /* ISC_H */
