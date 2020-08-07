/**
 * @file   shmem_rec.c
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   02 August 2020
 * @version 0.1
 * @brief   The shmem.so module acts as a bridge between the main isc process and the 
 *          consumer process on the same machine as isc.
 *          This module connects the Inter SoC Communication (ISC) system to the consumer
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
#include <stdbool.h>
#include <pwd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/shm.h>

#include "isc.h"

/*********************************************************************************** 
 * C o n s t a n t s ,   v a r i a b l e s ,  f u n c t i o n s 
************************************************************************************/

// The file to which to append the log string.
const char* log_filename = "shmem_rec.log";
FILE *log_fd = NULL;


///////////////////////////////////////////////////////////////////////////////////
// C o n s u m e r
///////////////////////////////////////////////////////////////////////////////////
#define BUFFER_INIT1 0x01
#define BUFFER_INIT2 0x02

// Shared Memory Key and Binary Semaphore
static key_t cons_semkey;
static int cons_semid;
static key_t cons_shmkey;
static int cons_shmid;
static char *cons_shm;
static int init1 = 1;


// Interface function as a constructor
void ipc_init (void (*ipc_rec)(unsigned char *buf, int bufSize), void (*ipc_xmit)(uint8_t *buf, int32_t bufSize))
{
  if (verbose)
    printf("\nshmem_rec - ipc_init\n");
  fprintf(main_log_fd, "\n%s - INFO - shmem_rec - ipc_init", get_timestamp());

  // Open the ipc log file for writing. If it exists, append to it;
  // otherwise, create a new file.
  log_fd = fopen (log_filename, "w");
  if (log_fd == NULL) {
    fprintf (stderr, "error: (%s) %s\n", "log_fd", strerror (errno));
  }


/////////////////////////////////////////
// C o n s u m e r
/////////////////////////////////////////

  // Get unique key for xmit shared memory
  if ((cons_shmkey = ftok("/tmp/cons_shmem_key", 'R')) == -1) // Here the file must exist 
    system_error ("ipc_init - cons_shmkey ftok");

  // Create the segment
  if ((cons_shmid = shmget(cons_shmkey, CONS_SHM_SIZE, 0644 | IPC_CREAT)) == -1)
    system_error ("ipc_init - cons_shmid shmget");

  // Attach to the segment to get a pointer to it
  cons_shm = shmat(cons_shmid, (void *)0, 0);
  if (cons_shm == (char *)(-1))
    system_error ("ipc_init - cons_shm shmat");

  // Get unique key for xmit semaphore
  if ((cons_semkey = ftok("/tmp/cons_sem_key", 'R')) == (key_t) -1)
    system_error ("ipc_init - cons_semkey ftok");

  // Allocate xmit semaphore
  if ((cons_semid = binary_semaphore_allocation (cons_semkey, 0644 | IPC_CREAT)) == -1 )
    system_error ("ipc_init - cons_semid binary_semaphore_allocation");

  // Init xmit semaphore
  if (binary_semaphore_initialize (cons_semid) == -1 )
    system_error ("ipc_init - cons_semid binary_semaphore_initialize");
}


// Interface function as a destructor
void ipc_cleanup ()
{
  if (verbose)
    printf("\nshmem_rec - ipc_cleanup\n");
  fprintf(main_log_fd, "\n%s - INFO - shmem_rec - ipc_cleanup", get_timestamp());

/////////////////////////////////////////
// C o n s u m e r
/////////////////////////////////////////
  // Detach from the segment
  if (shmdt(cons_shm) == -1)
    system_error ("ipc_cleanup - cons shmdt");

  // All done. Close the main log file.
  if (log_fd)
    fclose ((FILE*) log_fd);
}


// Interface function to xmit data
uint32_t ipc_xmit (uint8_t *buf, int32_t bufSize)
{
  system_error ("ipc_xmit - shmem_rec - Not implemented");
  return 0;
}


// Interface function to receive data
void ipc_rec (uint8_t *buf, int32_t bufSize)
{
  int i;
  if (verbose)
    printf("\nshmem_rec - ipc_rec");
  fprintf(main_log_fd, "\n%s - INFO - shmem_rec - ipc_rec", get_timestamp());

//  printf("\nshmem:ipc_rec:rec semaphore wait...\n");
  memcpy(cons_shm, (const char *)buf, bufSize);

  binary_semaphore_post(cons_semid);

  fprintf(log_fd, "\n%s - INFO - shmem_rec - ", get_timestamp());

#if 1
  for (i = 0; i < bufSize; i++) {
    fprintf(log_fd, "0x%X,", buf[i]);
  }
#endif
//  if (init1) {
//    for (i = 0; i < CONS_TEST_REGION_SIZE-1; i++)
//      cons_test_buff[i] = cons_test_buff[i]+ BUFFER_INIT2 + i;
//    init1 = 0;
//  } else {
//    for (i = 0; i < CONS_TEST_REGION_SIZE-1; i++)
//      cons_test_buff[i] = cons_test_buff[i] + BUFFER_INIT1 + i;
//    init1 = 1;
//  }

}


// Interface function to request stopping the thread
void ipc_stop()
{
  if (verbose)
    printf("\nshmem_rec - ipc_stop");
}


// Interface function to set configuration parameters
bool ipc_set_param(const char* prtcl, const char *addr, int port)
{
  if (verbose)
    printf("\nshmem_rec - ipc_set_param");
  return true;
}


// Interface function to start the thread 
bool ipc_start()
{
  if (verbose)
    printf("\nshmem_rec - ipc_start");
  return true;
}


// Interface function to join the thread 
bool ipc_wait4Done()
{
  if (verbose)
    printf("\nshmem_rec - ipc_wait4Done");
  return true;
}
