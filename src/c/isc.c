/**
 * @file   isc.c
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   04 August 2020
 * @version 0.1
 * @brief   isc.c is the implementation of the Inter SoC Communication (ISC) framework.
 *
 * These are the implemented functionalities in isc.c:
 * - isc_run is the main entry point for running the isc. This function starts
 *   the shared memory and server/client network threads.
 * - The shared memory xmitter/receiver modules are used for connection to another different
 *   process on the same machine as isc.
 * - In this implementation, the network server/client uses TCP socket for connection to 
 *   another SoC. It can be easily extended to other bus topologis like PCIe.
 * - isc_run installs clean up as the signal handler for SIGCHLD. It acts as a destructor
 *   for the whole system. This function simply cleans up terminated process.
 * - To gently and safely shutdown the system, first the stop requets are sent to each
 *   running thread, and then after joining all threads, the system is being destructed.
 * - As a flexibility, expandability, and scalability measure, the shared memort and networking
 *   modules are implemented as dynamically loadable modules at run-time. Basically, they act
 *   as plugins.
 */

#include <arpa/inet.h>
#include <assert.h>
#include <errno.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdbool.h>
#include <malloc.h>
#include <pthread.h>
#include <semaphore.h>
#include <stddef.h>
#include <stdint.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/utsname.h>
#include "isc.h"


/*********************************************************************************** 
 * C o n s t a n t s ,   v a r i a b l e s ,  f u n c t i o n s 
************************************************************************************/

// IPC shmem receiver
static char* ipc_name_shmem_rec = "shmem_rec";
// IPC shmem xmitg
static char* ipc_name_shmem_xmit = "shmem_xmit";

// IPC socket client
static char* ipc_name_sckt_client = "sckt_client";
// IPC socket server
static char* ipc_name_sckt_server = "sckt_server";

// Inter Process Communication module: Shared Memory
struct ipc_module* module_shmem = NULL;
// Inter Process Communication module: Shared Memory
struct ipc_module* module_sckt = NULL;



// Close the log file pointer THREAD_LOG.
void close_thread_log (void* thread_log)
{
  fclose ((FILE*) thread_log);
}


// Destructor function
static void cleanup(void)
{
  printf("\nisc - cleanup");
  fprintf(main_log_fd, "\n%s - INFO - isc - cleanup", get_timestamp());
  // We're done with the modules.
  if (module_sckt) {
    (*module_sckt->cleanup_function) ();
    ipc_close (module_sckt);
  }

  if (module_shmem) {
    (*module_shmem->cleanup_function) ();
    ipc_close (module_shmem);
  }

  // All done. Close the main log file.
  if (main_log_fd)
    fclose ((FILE*) main_log_fd);
}


// Load the dynamically loadable IPC module.
static struct ipc_module* load_ipc_module (char* ipcName)
{
  // IPC file name.
  char module_file_name[64];
  struct ipc_module* ipcMdl = NULL;

  // Construct the module name by appending ".so" to the name.
  snprintf (module_file_name, 
            sizeof (module_file_name),
            "%s.so", ipcName);

  // Try to open the IPC module.
  ipcMdl = ipc_open (module_file_name);
  
  if (ipcMdl == NULL) {
    char buffer[250]; 
    sprintf(buffer, "load_ipc_module - Failed to open IPC(%s).", module_file_name);
    fprintf(main_log_fd, "\n%s - ERROR - isc - Loading module %s was successful", get_timestamp(), module_file_name);    // Loading IPC modules.
    system_error (buffer);
  }

  printf("\nload_ipc_module - Loading module %s was successful.", module_file_name);
  fprintf(main_log_fd, "\n%s - INFO - isc - Loading module %s was successful", get_timestamp(), module_file_name);    // Loading IPC modules.

  return ipcMdl;
}


// Interrupt handler to force the transmitter/received threads for safely finishing.
void sigHandler(int sig)
{
  printf("\nisc - sigHandler");  
  fprintf(main_log_fd, "\n%s - INFO - isc - received signal", get_timestamp());    // Loading IPC modules.
 
  if (module_sckt) {
    (*module_sckt->stop_function) ();
  }
  if (module_shmem) {
    (*module_shmem->stop_function) ();
  }
}


// Main ISC core handler
void isc_run (const char* net_prtcl, const char* dest_ip_addr, int dest_port, int is_client)
{
  bool ok = true;
  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &sigHandler;
  sigaction (SIGUSR1, &sa, NULL);


  // Bind the destructor with exit situation
  atexit(cleanup);


  if (!is_client) {  // Server Operation Mode
    printf("\nisc - isc_run in server mode");
    fprintf(main_log_fd, "\n%s - INFO - isc - isc_run - server mode", get_timestamp());    // Loading IPC modules.
    // Loading IPC modules.
    module_shmem = load_ipc_module (ipc_name_shmem_rec);
    module_sckt = load_ipc_module (ipc_name_sckt_server);

    (*module_shmem->init_function) (NULL, NULL);
    (*module_sckt->init_function) (module_shmem->rec_function, NULL);
  }
  else {  // Client Operation Mode
    printf("\nisc - isc_run in client mode");
    fprintf(main_log_fd, "\n%s - INFO - isc - isc_run - client mode", get_timestamp());    // Loading IPC modules.
    module_shmem = load_ipc_module (ipc_name_shmem_xmit);
    module_sckt = load_ipc_module (ipc_name_sckt_client);

    (*module_sckt->init_function) (NULL, NULL);
    (*module_shmem->init_function) (NULL, module_sckt->xmit_function);
  }


  // Setting network parameters
  if ((*module_sckt->set_param_function) (net_prtcl, dest_ip_addr, dest_port) == false) ok = false;


  // Invoking the thread start interface for shared memory module
  if (ok) {
    if (!(*module_shmem->start_function) ()) ok = false;
  }


  // Invoking the thread start interface for network module
  if (ok) {
    if (!(*module_sckt->start_function) ()) ok = false;
  }


  // Waiting to join the network module thread
  if (ok) {
    if (!(*module_sckt->wait4Done_function) ()) { ok = false; system_error("isc - Failed to wait for Socket transport!"); }
    else fprintf(main_log_fd, "\n%s - INFO - isc - isc_run - Socket transport is successfuly killed.", get_timestamp());
  }


  // Waiting to join the shared memory module thread
  if (ok) {
    if (!(*module_shmem->wait4Done_function) ()) { ok = false; system_error("isc - Failed to wait for Shared Memory receiver!"); }
      else fprintf(main_log_fd, "\n%s - INFO - isc - isc_run - Shared Memory receiver is successfuly killed.", get_timestamp());
  }

}
