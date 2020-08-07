/**
 * @file   producer.c
 * @author Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   03 Apr 2018
 * @version 0.1
 * @brief   producer.c runs on the same machine as isc but on a different process. 
 *          It connects via shared memory to the client thread of Inter SoC 
 *          Communication (ISC) module and feeds the produced data to that process.
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

#define BUFFER_INIT1 0x01
#define BUFFER_INIT2 0x02

uint8_t *prod_test_buff = NULL;

// Shared Memory Key and Binary Semaphore
key_t prod_semkey;
int prod_semid;
key_t prod_shmkey;
int prod_shmid;
char *prod_shm;

// The file to which to append the log string.
const char* main_log_filename = "producer.log";
FILE *main_log_fd = NULL;


// Constructor routine
void ipc_init ()
{
  printf("\nproducer:ipc_init\n");
  fprintf(main_log_fd, "\n%s - INFO - producer - ipc_init", get_timestamp ());

  // Get unique key for shared memory; The /tmp/prod_shmem_key file must exist!
  if ((prod_shmkey = ftok("/tmp/prod_shmem_key", 'R')) == -1){
    system_error ("producer - prod_shmkey ftok");
  }


  // Create the segment
  if ((prod_shmid = shmget(prod_shmkey, PROD_SHM_SIZE, 0644 | IPC_CREAT)) == -1) {
    system_error ("producer - shmem create the segment");
  }

  // Attach to the segment to get a pointer to it
  prod_shm = shmat(prod_shmid, (void *)0, 0);
  if (prod_shm == (char *)(-1)) {
    system_error ("producer - shmem attach to the segment");
  }

  // Get unique key for semaphore.
  if ((prod_semkey = ftok("/tmp/prod_sem_key", 'R')) == (key_t) -1) {
    system_error ("producer - prod_sem_key ftok");
  }

  if ((prod_semid = binary_semaphore_allocation (prod_semkey, 0644 | IPC_CREAT)) == -1 ) {
    system_error ("producer - binary_semaphore_allocation");
  }

  if (binary_semaphore_initialize (prod_semid) == -1 ) {
    system_error ("producer - binary_semaphore_initialize: prod_semid");
  }
}


// Destructor helper routine
void ipc_cleanup ()
{
  printf("\nproducer:ipc_cleanup\n");
  fprintf(main_log_fd, "\n%s - INFO - producer - ipc_cleanup", get_timestamp ());

  //binary_semaphore_deallocate(prod_semid);

  // Detach from the segment
  if (shmdt(prod_shm) == -1) {
    system_error ("producer - Detach from the segment");
  }
}


// Destructor routine
static void free_all(void)
{
  ipc_cleanup();

  if (prod_test_buff)
    free(prod_test_buff);

  // All done. Close the main log file.
  fclose ((FILE*) main_log_fd);
}


// Main entry
int main(int argc, char *argv[])
{
  int i, j, init1 = 1;

  atexit(free_all);

  // Open the file for writing. If it exists, append to it;
  // otherwise, create a new file.
  main_log_fd = fopen (main_log_filename, "w");
  if (main_log_fd == NULL) {
    fprintf (stderr, "error: (%s) %s\n", "main_log_fd", strerror (errno));
  }

  ipc_init();

  prod_test_buff = (uint8_t *) malloc(PROD_TEST_REGION_SIZE);
  if (!prod_test_buff) {
    system_error ("producer - can't alloc the prod_test_buff.");
  }

  for (i = 0; i < PROD_TEST_REGION_SIZE-1; i++)
    prod_test_buff[i] = BUFFER_INIT1 + i;
  prod_test_buff[PROD_TEST_REGION_SIZE-1] = '\0';

  for (j = 0; j < 1; j++) {
    strncpy (prod_shm, prod_test_buff, PROD_SHM_SIZE);

    binary_semaphore_post (prod_semid);

    printf ("\n%s - INFO - Xmited(%i) - ", get_timestamp (), j);
    fprintf (main_log_fd, "\n%s - INFO - producer - Xmited(%i) - ", get_timestamp (), j);
#if 1    
    for (i = 0; i < PROD_TEST_REGION_SIZE; i++)
      fprintf (main_log_fd, "0x%X,", prod_shm[i] & 0x000000FF);
#endif
    if (init1) {
      for (i = 0; i < PROD_TEST_REGION_SIZE-1; i++)
        prod_test_buff[i] = prod_test_buff[i]+ BUFFER_INIT2 + i;
      init1 = 0;
    } else {
      for (i = 0; i < PROD_TEST_REGION_SIZE-1; i++)
        prod_test_buff[i] = prod_test_buff[i] + BUFFER_INIT1 + i;
      init1 = 1;
    }

    better_sleep (0.1);
  }

  printf ("\n");
  return 0;
}
