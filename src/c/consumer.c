/**
 * @file   consumer.c
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   02 August 2020
 * @version 0.1
 * @brief   consumer.c runs on the same machine as isc but on a different process. 
 *          It connects via shared memory to the server thread of Inter SoC 
 *          Communication (ISC) module and fetches data received data from that process.
 */

#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <pwd.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <stdint.h>
#include <signal.h>
#include "isc.h"


/*********************************************************************************** 
 * C o n s t a n t s ,   v a r i a b l e s ,  f u n c t i o n s 
************************************************************************************/

// Shared Memory Key and Binary Semaphore
key_t cons_semkey;
int cons_semid;
key_t cons_shmkey;
int cons_shmid;
char *cons_shm;

// The file to which to append the log string.
const char* main_log_filename = "consumer.log";
FILE *main_log_fd = NULL;

// Flag for terminating the program gracefully
// as it receives SIGUSR1
sig_atomic_t sigusr1_count = 0;


// Constructor routine
void ipc_init ()
{
  printf("\nconsumer - ipc_init\n");
  fprintf(main_log_fd, "\n%s - INFO - consumer - ipc_init", get_timestamp ());

  // Get unique key for xmit shared memory; The /tmp/cons_shmem_key file must exist!
  if ((cons_shmkey = ftok("/tmp/cons_shmem_key", 'R')) == -1) {
    system_error ("consumer - cons_shmem_key ftok");
  }

  // Create the segment
  if ((cons_shmid = shmget(cons_shmkey, CONS_SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
    system_error ("consumer - Create the segment shmem");
  }

  // Attach to the segment to get a pointer to it
  cons_shm = shmat(cons_shmid, (void *)0, 0);
  if (cons_shm == (char *)(-1)) {
    system_error ("consumer - Attach to the segment shmem");
  }

  // Get unique key for xmit semaphore
  if ((cons_semkey = ftok("/tmp/cons_sem_key", 'R')) == (key_t) -1) {
    system_error ("consumer - cons_sem_key ftok");
  }

  // Allocate xmit semaphore
  if ((cons_semid = binary_semaphore_allocation (cons_semkey, 0644 | IPC_CREAT)) == -1 ) {
    system_error ("consumer - binary_semaphore_allocation");
  }

  // Init xmit semaphore
  if (binary_semaphore_initialize (cons_semid) == -1 ) {
    system_error ("consumer - binary_semaphore_initialize");
  }
}


// Destructor helper routine
void ipc_cleanup ()
{
  printf ("\nconsumer - ipc_cleanup\n");
  fprintf (main_log_fd, "\n%s - INFO - consumer - ipc_cleanup", get_timestamp ());

  if (binary_semaphore_deallocate(cons_semid) == -1) {
    system_error ("consumer - binary_semaphore_deallocate");
  }

  // Detach from the xmit shared memory segment
  if (shmdt(cons_shm) == -1) {
    system_error ("consumer - Detach from the xmit shmem");
  }

  // Deallocate the xmit shared memory segment
  shmctl (cons_shmid, IPC_RMID, 0); 
}


// Destructor routine
static void free_all(void)
{
  ipc_cleanup();

  // All done.
  fclose ((FILE*) main_log_fd);  
}


// Interrupt handler to force free all resources safely.
void sigHandler(int sig)
{
  // Increment exit counter
  ++sigusr1_count;
}


// Main entry
int main(int argc, char *argv[])
{
  int s = 0, i, j = 0;

  struct sigaction sa;
  memset (&sa, 0, sizeof (sa));
  sa.sa_handler = &sigHandler;
  sigaction (SIGUSR1, &sa, NULL);

  atexit (free_all);

  // Open the file for writing. If it exists, append to it;
  // otherwise, create a new file.
  main_log_fd = fopen (main_log_filename, "w");
  if (main_log_fd == NULL) {
    fprintf (stderr, "error: (%s) %s\n", "main_log_fd", strerror (errno));
  }

  ipc_init();

  for ( ;; ) {
    // Finish loop as receives SIGUSR1
    if (sigusr1_count > 0)
      break;

    s = binary_semaphore_wait (cons_semid);
    // Check what happened
    if (s == -1) {
      if (errno == ETIMEDOUT) {
        printf ("\nconsumer - sem_timedwait() timed out\n");
        fprintf (main_log_fd, "\n%s - WARNING - consumer - sem_timedwait() timed out.", get_timestamp ());
      }
      else
        ;//perror("sem_timedwait");
    } else {
      printf ("\n%s - INFO - Reced(%i) - ", get_timestamp (), j);
      fprintf (main_log_fd, "\n%s - INFO - consumer - Reced(%i) - ", get_timestamp (), j);
#if 1
      for (i = 0; i < CONS_TEST_REGION_SIZE; i++) {
        fprintf (main_log_fd, "0x%X,", cons_shm[i] & 0x000000FF);
      }
#endif
      j++;
    }
  }

  printf ("\n");

  return 0;
}
