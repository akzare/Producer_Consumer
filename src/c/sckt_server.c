/**
 * @file   sckt_server.c
 * @author The concept of Linux Network programming based on epoll is used from:
 *           https://programmer.ink/think/epoll-for-linux-programming.html
 *         (multi-threading) Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   03 August 2020
 * @version 0.1
 * @brief   The sckt_server.so module acts as a bridge between two isc system on different 
 *          SoC modules. This module implements a server socket transport in Linux based 
 *          on epoll.
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
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <resolv.h>
#include <sys/epoll.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>    // NI_MAXHOST, NI_MAXSERV
#include <netinet/tcp.h>
#include <sys/epoll.h>

#include "isc.h"



/*********************************************************************************** 
 * C o n s t a n t s ,   v a r i a b l e s ,  f u n c t i o n s 
************************************************************************************/
#define LISTENQ 20

static const int EPOLL_MAXEVENTS = 128;
static const EPOLL_TIMEOUT = 1000;
#define IOBUFFSIZE 2048

// The file to which to append the log string.
static const char* log_filename = "sckt_server.log";
static FILE *log_fd = NULL;


// Thread Name
const char *threadNameListen  = "SocketListener";


static bool isListening;    // ListenerProc thread is running if this is true..
static int Protocol;
static int SocketType;
static int ConnPort;
static int AddrFamily;


static pthread_t ListenerProcID;

  
// Thread routines
static bool bListenerProcActive;
static void *scktListenerThread(void *pArg);
static void scktListenerProc(); // In Listen mode, handles new connections and inbound data.


// local helpers
static void setnonblocking(int sock);


// Callback function to feed data to the next chain in pipeline
void (*recCallbackFunctionType)(uint8_t *buf, int32_t bufSize);


// Interface function as a constructor
void ipc_init (void (*ipc_rec)(uint8_t *buf, int32_t bufSize), void (*ipc_xmit)(uint8_t *buf, int32_t bufSize))
{
  if (verbose)
    printf("\nsckt_server - ipc_init\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_server - ipc_init", get_timestamp());

  // Open the ipc log file for writing. If it exists, append to it;
  // otherwise, create a new file.
  log_fd = fopen (log_filename, "w");
  if (log_fd == NULL) {
    fprintf (stderr, "error: (%s) %s\n", "log_fd", strerror (errno));
  }

  recCallbackFunctionType = ipc_rec;

  isListening = false;

  bListenerProcActive = false;
 
  return;
}


// Interface function as a destructor
void ipc_cleanup ()
{
  if (verbose)
    printf("\nsckt_server - ipc_cleanup\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_server - ipc_cleanup", get_timestamp());

  // All done. Close the main log file.
  if (log_fd)
    fclose ((FILE*) log_fd);
}


// Interface function to request stopping the thread
void ipc_stop()
{
  if (verbose)
    printf("\nsckt_server - ipc_stop\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_server - ipc_stop", get_timestamp());

  isListening = false;
}
  

// Interface function to set configuration parameters
bool ipc_set_param(const char* prtcl, const char *addr, int port)
{
  bool rval = true;

  if (verbose)
    printf("\nsckt_server - ipc_set_param\n");

  if (strcmp(prtcl, "tcp") == 0) {
    Protocol = IPPROTO_TCP;
  }
  else if (strcmp(prtcl, "udp") == 0) {
    Protocol = IPPROTO_UDP;
  }
  else
    rval = false;

  if (rval) {
    SocketType = SOCK_STREAM;
    ConnPort = port; // use PF_INET domain socket connection
    AddrFamily = AF_INET;
  }

  if (rval == true)
    fprintf(main_log_fd, "\n%s - INFO - sckt_server - ipc_set_param - Protocol:%s, SocketType:SOCK_STREAM, PORT:%d.", get_timestamp(), prtcl, ConnPort);
  else
    fprintf(main_log_fd, "\n%s - ERROR - sckt_server - ipc_set_param - Protocol:%s, SocketType:SOCK_STREAM, PORT:%d.", get_timestamp(), prtcl, port);

  return (rval);
}


// Interface function to start the thread 
bool ipc_start()
{
  if (verbose)
    printf("\nsckt_server - ipc_start\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_server - ipc_start", get_timestamp());


  // ////////////////////////////////
  // Create the thread
  pthread_attr_t thread_attribs;
  pthread_attr_init(&thread_attribs);
  pthread_attr_setscope(&thread_attribs, PTHREAD_SCOPE_SYSTEM);
//  pthread_attr_setdetachstate(&thread_attribs,PTHREAD_CREATE_DETACHED);
  pthread_attr_setstacksize(&thread_attribs, 65536);

  bListenerProcActive = true;
  isListening  = true;

  if ( pthread_create(&ListenerProcID, &thread_attribs, scktListenerThread, NULL) ) {
    isListening = false;
    system_error("ipc_start - scktStartListener: error creating listener thread, aborting");
  }
  else {
    pthread_setname_np(ListenerProcID, threadNameListen);
    while(!bListenerProcActive) { usleep(100); } // wait for the thread to come up
  }
   
  pthread_attr_destroy(&thread_attribs);

  return (isListening);
}


// Interface function to join the thread 
bool ipc_wait4Done()
{
  if (verbose)
    printf("\nsckt_server - ipc_wait4Done\n");

  // Make sure the listener thread has finished.
  if (pthread_join(ListenerProcID, NULL) != 0) return false;

  fprintf(main_log_fd, "\n%s - INFO - sckt_server - wait4Done - ListenerProc exited", get_timestamp());

  return true;
}


// Set socket to Non-Blocking mode
void setnonblocking(int sock)
{
  int opts;
  opts=fcntl(sock,F_GETFL);
  if (opts < 0) {
    perror("fcntl(sock,GETFL)");
    exit(1);
  }
  opts = opts|O_NONBLOCK;
  if(fcntl(sock,F_SETFL,opts) < 0) {
    perror("fcntl(sock,SETFL,opts)");
    exit(1);
  }
}


// Socket listener main thread
void *scktListenerThread(void *pArg)
{
  if (verbose)
    printf("\nsckt_server - scktListenerThread - listenerproc started\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_server - listenerproc started", get_timestamp());

  bListenerProcActive = true;
  scktListenerProc();
  isListening = false;
  bListenerProcActive = false;

  fprintf(main_log_fd, "\n%s - INFO - sckt_server - listenerproc exited", get_timestamp());
}


// Socket listener main thread process
void scktListenerProc()
{
  int i, listenfd, connfd, sockfd, epfd, nfds;
  ssize_t n;
  socklen_t clilen;
  unsigned char buf[IOBUFFSIZE];

//Declare variables for the epoll_event structure, ev for registering events, and array for returning events to process

  struct epoll_event ev, events[20];
  //Generate epoll-specific file descriptors for processing accept s

  epfd = epoll_create(256);
  struct sockaddr_in clientaddr;
  struct sockaddr_in serveraddr;
  listenfd = socket(AF_INET, SOCK_STREAM, (int)Protocol);
  //Set socket to non-blocking

  setnonblocking(listenfd);

  //Set the file descriptor associated with the event to be processed

  ev.data.fd = listenfd;
  //Set the type of event to process

  ev.events = EPOLLIN | EPOLLET;
  //ev.events = EPOLLIN;

  //Register epoll events

  epoll_ctl(epfd, EPOLL_CTL_ADD, listenfd,&ev);
  bzero(&serveraddr, sizeof(serveraddr));
  serveraddr.sin_family = AF_INET;
  serveraddr.sin_addr.s_addr = INADDR_ANY;
  serveraddr.sin_port = htons(ConnPort);

  bind(listenfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr));
  listen(listenfd, LISTENQ);

  isListening = true;

  while (isListening) {
    //Waiting for the epoll event to occur

    nfds = epoll_wait(epfd, events, EPOLL_MAXEVENTS, EPOLL_TIMEOUT);
    //Handle all events that occur

    for (i = 0; i < nfds; ++i) {
      if (events[i].data.fd == listenfd) {//If a new SOCKET user is detected to be connected to a bound SOCKET port, establish a new connection.
            
        connfd = accept(listenfd, (struct sockaddr *)&clientaddr, &clilen);
        if (connfd < 0) {
          perror("sckt_server - connfd<0");
          exit(1);
        }
        //setnonblocking(connfd);

        char *str = inet_ntoa(clientaddr.sin_addr);
        if (verbose)
          printf("sckt_server - Accapt a connection from %s\n", str);
        fprintf(main_log_fd, "\n%s - INFO - sckt_server - Accapt a connection from %s", get_timestamp(), str);
        //Setting file descriptors for read operations

        ev.data.fd = connfd;
        //Set Read Action Events for Annotation

        ev.events = EPOLLIN|EPOLLET;
        //ev.events=EPOLLIN;

        //Register ev

        epoll_ctl(epfd,EPOLL_CTL_ADD, connfd, &ev);
      }
      else if (events[i].events & EPOLLIN) {//If the user is already connected and receives data, read in.
        if ( (sockfd = events[i].data.fd) < 0)
          continue;


        if ( (n = read(sockfd, buf, IOBUFFSIZE)) < 0) {
          if (errno == ECONNRESET) {
            close(sockfd);
            events[i].data.fd = -1;
          } else
            printf("sckt_server - ERROR - read error\n");
            fprintf(main_log_fd, "\n%s - ERROR - sckt_server - read error", get_timestamp());
        } else if (n == 0) {
          close(sockfd);
          events[i].data.fd = -1;
        }
        else {
          fprintf(log_fd, "\n%s - INFO - sckt_server - ", get_timestamp());

#if 1
//    printf("Shared memory contains: \"%s\"\n", prod_shm);
          for (i = 0; i < n; i++) {
            fprintf(log_fd, "0x%X,", buf[i] & 0x000000FF);
          }
#endif
          recCallbackFunctionType(buf, n);
        }

        // Setting file descriptors for write operations
        ev.data.fd = sockfd;
        // Set Write Action Events for Annotation

        ev.events = EPOLLOUT | EPOLLET;
        // Modify the event to be handled on sockfd to EPOLLOUT

        // epoll_ctl(epfd,EPOLL_CTL_MOD,sockfd,&ev);
      }
      else if(events[i].events&EPOLLOUT) { // If there is data to send
 
        sockfd = events[i].data.fd;

        // ToDo: Implement the xmit functionality
//      write(sockfd, ..., ...);
        // Setting file descriptors for read operations

        ev.data.fd = sockfd;
        // Set Read Action Events for Annotation

        ev.events = EPOLLIN|EPOLLET;
        // Modify the event to be processed on sockfd to EPOLIN

        epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, &ev);
      }
    }
  }

  close(listenfd);

  fprintf(main_log_fd, "\n%s - INFO - sckt_server - Listener exiting", get_timestamp());
}


// Interface function to xmit data
uint32_t ipc_xmit (uint8_t *buf, int32_t bufSize)
{ 
  system_error ("sckt_server - ipc_xmit - Not implemented");
  return 0;
}


// Interface function to receive data
void ipc_rec (uint8_t *buf, int32_t bufSize)
{
  system_error ("sckt_server - ipc_rec - Not implemented");
}
