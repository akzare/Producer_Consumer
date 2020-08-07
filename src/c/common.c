/**
 * @file   common.c
 * @author The main body of this code is from Advanced Linux Programming book by 
 *         Mark Mitchell, Jeffrey Oldham, and Alex Samuel.
 *         (further development) Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   01 August 2020
 * @version 0.1
 * @brief   Contains functions of general utility that are used throughout the program.
 */

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <time.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/stat.h>
#include "isc.h"


// Program name
const char* program_name;


// Verbose global flag: To increase the verbosity of the program it should be
// set to 1. By default the verbosity is deactivated.
int verbose;


// /////////////////////////////////////////////////////////
// M E M O R Y
// /////////////////////////////////////////////////////////

// A wrapper function around system malloc method to hanle NULL checking
void* xmalloc (size_t size)
{
  void* ptr = malloc (size);

  // Abort if the allocation failed.
  if (ptr == NULL)
    abort ();
  else
    return ptr;
}


// A wrapper function around system realloc method to hanle NULL checking
void* xrealloc (void* ptr, size_t size)
{
  ptr = realloc (ptr, size);

  // Abort if the allocation failed.
  if (ptr == NULL)
    abort ();
  else
    return ptr;
}


// A wrapper function around system strdup method to hanle NULL checking
char* xstrdup (const char* s)
{
  char* copy = strdup (s);

  // Abort if the allocation failed.
  if (copy == NULL)
    abort ();
  else
    return copy;
}


// /////////////////////////////////////////////////////////
// E R R O R   H A N D L I N G
// /////////////////////////////////////////////////////////


// A wrapper function around error function to append system errors to error message
void system_error (const char* operation)
{
  // Generate an error message for errno.
  error (operation, strerror (errno));
}


// A utility function to handle errors by adding reports into main log file
void error (const char* cause, const char* message)
{
  // Print an error message to stderr.
  fprintf (stderr, "%s: error: (%s) %s\n", program_name, cause, message);
  if (main_log_fd != NULL && main_log_fd > 0)
    fprintf(main_log_fd, "\n%s - ERROR - %s - %s", get_timestamp (), cause, message);

  // End the program.
  exit (1);
}


// /////////////////////////////////////////////////////////
// F I L E   S Y S T E M
// /////////////////////////////////////////////////////////

// Get the executable file current directory
char* get_self_executable_directory ()
{
  int rval;
  char link_target[1024];
  char* last_slash;
  size_t result_length;
  char* result;

  // Read the target of the symbolic link /proc/self/exe.
  rval = readlink ("/proc/self/exe", link_target, sizeof (link_target));
  if (rval == -1)
    // The call to readlink failed, so bail.
    abort ();
  else
    // NULL-terminate the target.
    link_target[rval] = '\0';

  // We want to trim the name of the executable file, to obtain the
  // directory that contains it. Find the rightmost slash.
  last_slash = strrchr (link_target, '/');
  if (last_slash == NULL || last_slash == link_target)
    // Something strange is going on.
    abort ();

  // Allocate a buffer to hold the resulting path.
  result_length = last_slash - link_target;
  result = (char*) xmalloc (result_length + 1);

  // Copy the result.
  strncpy (result, link_target, result_length);
  result[result_length] = '\0';

  return result;
}



// /////////////////////////////////////////////////////////
// B I N A R Y   S E M A P H O R E
// /////////////////////////////////////////////////////////

// Define union semun.
union semun {
  int val;
  struct semid_ds *buf;
  unsigned short int *array;
  struct seminfo *__buf;
};

int binary_semaphore_allocation (key_t key, int sem_flags)
{
  int semid;

  // Get semaphore ID associated with this key
  if ((semid = semget (key, 0, 0)) == -1){
    return semget (key, 1, sem_flags);
  }
  return semid;
}

int binary_semaphore_deallocate (int semid)
{
  union semun ignored_argument;
  return semctl (semid, 1, IPC_RMID, ignored_argument);
}

int binary_semaphore_initialize (int semid)
{
  union semun argument;
  unsigned short values[1];

  values[0] = 1;
  argument.array = values;
  return semctl (semid, 0, SETALL, argument);
}

int binary_semaphore_wait (int semid)
{
  struct sembuf operations[1];

  // Use the first (and only) semaphore.
  operations[0].sem_num = 0;
  // Decrement by 1.
  operations[0].sem_op = -1;
  // Permit undo’ing.
  operations[0].sem_flg = SEM_UNDO;
//  return semop (semid, operations, 1);
  struct timespec ts = { 0, 1*1000000 }; // 1 ms
  return semtimedop (semid, operations, 1, &ts);
}

int binary_semaphore_post (int semid)
{
  struct sembuf operations[1];
  
  // Use the first (and only) semaphore.
  operations[0].sem_num = 0;
  // Increment by 1.
  operations[0].sem_op = 1;
  // Permit undo'ing.
  operations[0].sem_flg = SEM_UNDO;
  return semop (semid, operations, 1);
}


// /////////////////////////////////////////////////////////
// S L E E P   &   T I M E
// /////////////////////////////////////////////////////////

// A utility wrapper function around system nanosleep method
int better_sleep (double sleep_time)
{
  struct timespec tv;
  // Construct the timespec from the number of whole seconds...
  tv.tv_sec = (time_t) sleep_time;
  // ... and the remainder in nanoseconds.
  tv.tv_nsec = (long) ((sleep_time - tv.tv_sec) * 1e+9);
  while (1)
  {
    // Sleep for the time specified in tv. If interrupted by a
    // signal, place the remaining time left to sleep back into tv.
    int rval = nanosleep (&tv, &tv);
    if (rval == 0)
      // Completed the entire sleep time; all done.
      return 0;
    else if (errno == EINTR)
      // Interrupted by a signal. Try again.
      continue;
    else
      // Some other error; bail out.
      return rval;
    }
    return 0;
}


// Utility function to print time
void print_time ()
{
  struct timeval tv;
  struct tm* ptm;
  char time_string[40];
  long milliseconds;

  // Obtain the time of day, and convert it to a tm struct.
  gettimeofday (&tv, NULL);
  ptm = localtime (&tv.tv_sec);
  // Format the date and time, down to a single second.
  strftime (time_string, sizeof (time_string), "%Y-%m-%d %H:%M:%S", ptm);
  // Compute milliseconds from microseconds.
  milliseconds = tv.tv_usec / 1000;
  // Print the formatted time, in seconds, followed by a decimal point
  // and the milliseconds.
  printf ("%s.%03ld\n", time_string, milliseconds);
}

char *asctime1(const struct tm *timeptr)
{
  static char wday_name[7][3] = {
    "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat"
  };
  static char mon_name[12][3] = {
    "Jan", "Feb", "Mar", "Apr", "May", "Jun",
    "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
  };
  static char result[26];

//  sprintf(result, "%.3s %.3s %d %3d %.2d:%.2d:%.2d",
//        wday_name[timeptr->tm_wday],
//        mon_name[timeptr->tm_mon],
//        timeptr->tm_mday,
//        1900 + timeptr->tm_year,
//        timeptr->tm_hour,
//        timeptr->tm_min, timeptr->tm_sec
//        );

  sprintf(result, "%.3s.%d.%3d %.2d:%.2d:%.2d",
        mon_name[timeptr->tm_mon],
        timeptr->tm_mday,
        1900 + timeptr->tm_year,
        timeptr->tm_hour,
        timeptr->tm_min, 
        timeptr->tm_sec
        );

  return result;
}


// Return a character string representing the current date and time.
char* get_timestamp ()
{
  struct timeval tv;
  struct tm* ptm;
  long milliseconds;
  static char result[40];

  static char mon_name[12][3] = {
    "01", "02", "03", "04", "05", "06",
    "07", "08", "09", "10", "11", "12"
  };

  // Obtain the time of day, and convert it to a tm struct.
  gettimeofday (&tv, NULL);
  ptm = localtime (&tv.tv_sec);
  // Compute milliseconds from microseconds.
  milliseconds = tv.tv_usec;// / 1000;

  // Format the date and time, down to a single second.
  sprintf(result, "%.4d-%.2s-%.2d %.2d:%.2d:%.2d,%03ld",
        1900 + ptm->tm_year,
        mon_name[ptm->tm_mon],
        ptm->tm_mday,        
        ptm->tm_hour,
        ptm->tm_min, 
        ptm->tm_sec,
        milliseconds
        );

  return result;
}
