/**
 * @file   shmem_xmit.c
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   03 August 2020
 * @version 0.1
 * @brief   The shmem_xmit.so module acts as a bridge between the main isc process and the 
 *          producer process on the same machine as isc.
 *          This module connects the Inter SoC Communication (ISC) system to the producer 
 *          process via shared memory. 
*/

#include <string.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h> 
#include <pwd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/shm.h>

#include "isc.h"

/*********************************************************************************** 
 * C o n s t a n t s ,   v a r i a b l e s ,  f u n c t i o n s 
************************************************************************************/

// The file to which to append the log string.
const char* log_filename = "shmem_xmit.log";
FILE *log_fd = NULL;


///////////////////////////////////////////////////////////////////////////////////
// P r o d u c e r
///////////////////////////////////////////////////////////////////////////////////

// Shared Memory Key and Binary Semaphore
static key_t prod_semkey;
static int prod_semid;
static key_t prod_shmkey;
static int prod_shmid;
static char *prod_shm;
static bool stop;


// Xmitter process thread ID
static pthread_t XmitProcID;
 
const char *threadNameXmit  = "ShmemXmit";
// Thread routines
static bool XmitProcActive;
static void *xmitThread(void *pArg);
static void xmitProc(); // In Listen mode, handles new connections and inbound data.


// Callback function to feed data to the next chain in pipeline
int32_t (*xmitCallbackFunction)(uint8_t *buf, int32_t bufSize);


// Interface function as a constructor
void ipc_init (void (*ipc_rec)(unsigned char *buf, int bufSize), void (*ipc_xmit)(uint8_t *buf, int32_t bufSize))
{
  if (verbose)
    printf("\nshmem_xmit - ipc_init\n");
  fprintf(main_log_fd, "\n%s - INFO - shmem_xmit - ipc_init", get_timestamp());

  // Open the ipc log file for writing. If it exists, append to it;
  // otherwise, create a new file.
  log_fd = fopen (log_filename, "w");
  if (log_fd == NULL) {
    fprintf (stderr, "error: (%s) %s\n", "log_fd", strerror(errno));
  }

  // Assign the callback
  if (ipc_xmit != NULL)
    xmitCallbackFunction = ipc_xmit;
  else
    system_error ("shmem_xmit - xmitCallbackFunction is NULL");

  stop = false;
  XmitProcActive = false;

/////////////////////////////////////////
// P r o d u c e r
/////////////////////////////////////////

  // Get unique key for xmit shared memory
  if ((prod_shmkey = ftok("/tmp/prod_shmem_key", 'R')) == -1) // Here the file must exist 
    system_error ("shmem_xmit - prod_shmkey ftok");

  // Create the segment
  if ((prod_shmid = shmget(prod_shmkey, PROD_SHM_SIZE, 0644 | IPC_CREAT)) == -1)
    system_error ("shmem_xmit - prod_shmid shmget");

  // Attach to the segment to get a pointer to it
  prod_shm = shmat(prod_shmid, (void *)0, 0);
  if (prod_shm == (char *)(-1))
    system_error ("shmem_xmit - prod_shm shmat");

  // Get unique key for xmit semaphore
  if ((prod_semkey = ftok("/tmp/prod_sem_key", 'R')) == (key_t) -1)
    system_error ("shmem_xmit - prod_semkey ftok");

  // Allocate xmit semaphore
  if ((prod_semid = binary_semaphore_allocation (prod_semkey, 0644 | IPC_CREAT)) == -1 )
    system_error ("shmem_xmit - prod_semid binary_semaphore_allocation");

  // Init xmit semaphore
  if (binary_semaphore_initialize (prod_semid) == -1 )
    system_error ("shmem_xmit - xmit binary_semaphore_initialize");
}


// Interface function as a destructor
void ipc_cleanup ()
{
  if (verbose)
    printf("\nshmem_xmit - ipc_cleanup\n");
  fprintf(main_log_fd, "\n%s - INFO - shmem_xmit - ipc_cleanup", get_timestamp());

/////////////////////////////////////////
// P r o d u c e r
/////////////////////////////////////////
  if (binary_semaphore_deallocate(prod_semid) == -1)
    system_error ("shmem_xmit - ipc_cleanup - prod binary_semaphore_deallocate");

  // Detach from the xmit shared memory segment
  if (shmdt(prod_shm) == -1)
    system_error ("shmem_xmit - ipc_cleanup - prod shmdt");

  // Deallocate the xmit shared memory segment
  shmctl (prod_shmid, IPC_RMID, 0); 

  // All done. Close the main log file.
  if (log_fd)
    fclose ((FILE*) log_fd);
}


// Interface function to xmit data
uint32_t ipc_xmit (uint8_t *buf, int32_t bufSize)
{
  system_error ("shmem_xmit - ipc_xmit - Not implemented");
  return 0;
}


// Interface function to receive data
void ipc_rec (uint8_t *buf, int32_t bufSize)
{
  system_error ("shmem_xmit - ipc_rec - Not implemented");
}


// Interface function to request stopping the thread
void ipc_stop()
{
  if (verbose)
    printf("\nshmem_xmit - ipc_stop");
  fprintf(main_log_fd, "\n%s - INFO - shmem_xmit - ipc_stop", get_timestamp());
  stop = true;
}


// Interface function to set configuration parameters
bool ipc_set_param(const char* prtcl, const char *addr, int port)
{
  if (verbose)
    printf("\nshmem_xmit - ipc_set_param");
  return true;
}


// Interface function to start the thread 
bool ipc_start()
{
  bool rval = false;  
  if (verbose)
    printf("\nshmem_xmit - ipc_start");
  fprintf(main_log_fd, "\n%s - INFO - shmem_xmit - ipc_start", get_timestamp());

  // ////////////////////////////////
  // Create the thread
  pthread_attr_t thread_attribs;
  pthread_attr_init(&thread_attribs);
  pthread_attr_setscope(&thread_attribs, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setstacksize(&thread_attribs, 65536);

  if (pthread_create(&XmitProcID, &thread_attribs, xmitThread, NULL)) {
    system_error("shmem_xmit - error creating xmit thread, aborting");
  }
  else {
    pthread_setname_np(XmitProcID, threadNameXmit);
    while(!XmitProcActive) { usleep(100); } // wait for the thread to come up
    rval = true;
  }
   
  pthread_attr_destroy(&thread_attribs);

  return true;
}


// Interface function to join the thread 
bool ipc_wait4Done()
{
  if (verbose)
    printf("\nshmem_xmit - ipc_wait4Done");
 // Make sure the listener thread has finished.
  if (pthread_join(XmitProcID, NULL) != 0) return false;

  fprintf(main_log_fd, "\n%s - INFO - shmem_xmit - Wait4Done - XmitProc exited", get_timestamp());
  return true;
}


// Shared Mem Xmitter main thread
void *xmitThread(void *pArg)
{
  if (verbose)
    printf("\nshmem_xmit - xmitThread - xmitproc started...\n");
  fprintf(main_log_fd, "\n%s - INFO - shmem_xmit - xmitThread - xmitproc started", get_timestamp());

  XmitProcActive = true;
  xmitProc();
  XmitProcActive = false;

  fprintf(main_log_fd, "\n%s - INFO - shmem_xmit - xmitThread - xmitproc exited", get_timestamp());

  return (0);
}


// Shared Mem Xmitter main thread process
void xmitProc()
{
  if (verbose)
    printf("\nshmem_xmit - xmitProc starts");
  int s = 0, i;

  while (!stop) {
//    printf("\nshmem:ipc_xmit:xmit semaphore wait...\n");
    s = binary_semaphore_wait(prod_semid);
    // Check what happened
    if (s == -1) {
      if (errno == ETIMEDOUT) {
        printf("\nshmem_xmit - ipc_xmit: sem_timedwait() timed out\n");
        fprintf (main_log_fd, "\n%s - WARNING - shmem_xmit - sem_timedwait() timed out", get_timestamp());
      }
      else
        ;//perror("sem_timedwait");
    } 
    else {
      if (verbose)
        printf("\nshmem_xmit - ipc_xmit");

      fprintf(log_fd, "\n%s - INFO - shmem_xmit - ", get_timestamp());

#if 1
//    printf("Shared memory contains: \"%s\"\n", prod_shm);
      for (i = 0; i < PROD_TEST_REGION_SIZE; i++) {
        fprintf(log_fd, "0x%X,", prod_shm[i] & 0x000000FF);
      }
#endif
      
      // Callback the next node in the pipeline chain
      int32_t s = xmitCallbackFunction(prod_shm, PROD_TEST_REGION_SIZE);
      if (s != PROD_TEST_REGION_SIZE)
        printf ("\nshmem_xmit - Failed to write to the xmitter.");

    }
  }
  if (verbose)
    printf("\nshmem_xmit - xmitProc exits");
}
