/**
 * @file   sckt_client.c
 * @author The concept of Linux Network programming based on epoll is used from:
 *           https://programmer.ink/think/epoll-for-linux-programming.html
 *           https://stackoverflow.com/questions/10187347/async-connect-and-disconnect-with-epoll-linux
 *         (multi-threading) Armin Zare Zadeh ali.a.zarezadeh@gmail.com
 * @date   04 August 2020
 * @version 0.1
 * @brief   This module acts as a bridge between the internal producer/consumer 
 *          processes and external SoC module.
 *          Internally, this module connects to the transmitter shared memory and
 *          connects to the sckt_server module externally.
 *          This module implements epoll socket client thread.
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
#define MAXBUF 1024
#define MAX_EPOLL_EVENTS 64

static const int MAXEVENTS = 128;
#define IOBUFFSIZE 2048


// The file to which to append the log string.
static const char* log_filename = "sckt_client.log";
static FILE *log_fd = NULL;


// Client Thread Name
static const char *threadNameClient  = "SocketClient";

// Flag to indicate whether client is connected,
// or if it is already connected, should continue
static bool Connected;


// Transport type TCP/UDP
static int Protocol;
// Server IP address to connect to
static char *ServerAddr;
// Server port number to connect to
static int ConnPort;
// Socket address family
static int AddrFamily;

// Client socket file descriptor
static int client_sockfd = 0;
// Client process ID
static pthread_t ClientProcID;
 

// Thread routines
static bool ClientProcActive;
static void *scktClientThread(void *pArg);
static void scktClientProc(); // In Listen mode, handles new connections and inbound data.


// Callback function to feed data to the next chain in pipeline
void (*recCallbackFunctionType)(uint8_t *buf, int32_t bufSize);


// Interface function as a constructor
void ipc_init (void (*ipc_rec)(uint8_t *buf, int32_t bufSize), void (*ipc_xmit)(uint8_t *buf, int32_t bufSize))
{
  if (verbose)
    printf("\nsckt_client - ipc_init\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_client - ipc_init", get_timestamp());

  // Open the ipc log file for writing. If it exists, append to it;
  // otherwise, create a new file.
  log_fd = fopen (log_filename, "w");
  if (log_fd == NULL) {
    fprintf (stderr, "error: (%s) %s\n", "log_fd", strerror (errno));
  }

  recCallbackFunctionType = ipc_rec;
  client_sockfd = 0;
  Connected = false;

  ClientProcActive = false;
}


// Interface function as a destructor
void ipc_cleanup ()
{
  if (verbose)
    printf("\nsckt_client - ipc_cleanup\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_client - ipc_cleanup", get_timestamp());

  if (client_sockfd>0) close(client_sockfd);

  // All done. Close the main log file.
  if (log_fd)
    fclose ((FILE*) log_fd);
}


// Interface function to request stopping the thread
void ipc_stop()
{
  if (verbose)
    printf("\nsckt_client - ipc_stop\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_client - ipc_stop", get_timestamp());

  Connected = false;
}
  

// Interface function to set configuration parameters
bool ipc_set_param(const char* prtcl, const char *addr, int port)
{
  bool rval = true;

  if (verbose)
    printf("\nsckt_client - ipc_set_param\n");

  if (addr != NULL) {
    ServerAddr = strdup(addr);
  } 
  else {
    rval = false;
  }

  if (strcmp(prtcl, "tcp") == 0) {
    Protocol = IPPROTO_TCP;
  }
  else if (strcmp(prtcl, "udp") == 0) {
    Protocol = IPPROTO_UDP;
  }
  else
    rval = false;

  if (rval) {
    ConnPort = port; // use PF_INET domain socket connection
    AddrFamily = AF_INET;
  }

  if (rval == true)
    fprintf(main_log_fd, "\n%s - INFO - sckt_client - ipc_set_param - Protocol:%s, SocketType:SOCK_STREAM, ADDR:%s, PORT:%d.", get_timestamp(), prtcl, addr, ConnPort);
  else
    fprintf(main_log_fd, "\n%s - ERROR - sckt_client - ipc_set_param - Protocol:%s, SocketType:SOCK_STREAM, ADDR:%s, PORT:%d.", get_timestamp(), prtcl, addr, port);
  return (rval);
}


// Interface function to start the thread 
bool ipc_start()
{
  if (verbose)
    printf("\nsckt_client - ipc_start\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_client - ipc_start", get_timestamp());
  bool rval = false;


  // ////////////////////////////////
  // Create the thread
  pthread_attr_t thread_attribs;
  pthread_attr_init(&thread_attribs);
  pthread_attr_setscope(&thread_attribs, PTHREAD_SCOPE_SYSTEM);
  pthread_attr_setstacksize(&thread_attribs, 65536);

  if (pthread_create(&ClientProcID, &thread_attribs, scktClientThread, NULL)) {
    Connected = false;
    system_error("sckt_client - error creating client thread, aborting");
  }
  else {
    pthread_setname_np(ClientProcID, threadNameClient);
    while(!ClientProcActive) { usleep(100); } // wait for the thread to come up
    rval = true;
  }
   
  pthread_attr_destroy(&thread_attribs);

  return (rval);
}


// Interface function to join the thread 
bool ipc_wait4Done()
{
  if (verbose)
    printf("\nsckt_client - ipc_wait4Done\n");

  // Make sure the listener thread has finished.
  if (pthread_join(ClientProcID, NULL) != 0) return false;

  fprintf(main_log_fd, "\n%s - INFO - sckt_client - wait4Done - ClientProc exited", get_timestamp());

  return true;
}


// Socket Client main thread
void *scktClientThread(void *pArg)
{
  if (verbose)
    printf("\nsckt_client - scktClientThread - clientproc started\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_client - clientproc started", get_timestamp());

  ClientProcActive = true;
  scktClientProc();
  ClientProcActive = false;

  fprintf(main_log_fd, "\n%s - INFO - sckt_client - clientproc exited", get_timestamp());
}


// Socket Clinet main thread process
void scktClientProc()
{
  if (verbose)
    printf("\nsckt_client - scktClientProc starts");

  // ///////////////////////////////////
  // Connect Block Starts Here
  {
  int len; 
  struct sockaddr_in address; // Server-side Network Address Structures 
  int result; 

  if (verbose)
    printf("\nsckt_client - Trying to connect");

  client_sockfd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0); // Set up client socket 
  address.sin_family = AF_INET;
  address.sin_port = htons(ConnPort);
  if (inet_pton(AF_INET, ServerAddr, &address.sin_addr.s_addr) == 0) {
    perror(ServerAddr);
    exit(errno);
  }
  len = sizeof(address); 
  result = connect(client_sockfd, (struct sockaddr *)&address, len); 

  int retVal = -1;
  socklen_t retValLen = sizeof(retVal);

  if (result == 0) {
    // OK -- socket is ready for IO
    if (verbose)
      printf("\nsckt_client - socket is ready for IO");
    fprintf(main_log_fd, "\n%s - INFO - sckt_client - socket is ready for IO", get_timestamp());
  }
  else if (errno == EINPROGRESS) {
    struct epoll_event newPeerConnectionEvent;
    int epollFD = -1;
    struct epoll_event processableEvents;
    unsigned int numEvents = -1;

    if ((epollFD = epoll_create (1)) == -1) {
      printf ("\nsckt_client - ERROR - Could not create the epoll FD list. Aborting!");
      fprintf(main_log_fd, "\n%s - ERROR - sckt_client - Could not create the epoll FD list. Aborting", get_timestamp());
      exit (2);
    }     

    newPeerConnectionEvent.data.fd = client_sockfd;
    newPeerConnectionEvent.events = EPOLLOUT | EPOLLIN | EPOLLERR;

    if (epoll_ctl (epollFD, EPOLL_CTL_ADD, client_sockfd, &newPeerConnectionEvent) == -1) {
      printf ("\nsckt_client - ERROR - Could not add the socket FD to the epoll FD list. Aborting!");
      fprintf(main_log_fd, "\n%s - ERROR - sckt_client - Could not add the socket FD to the epoll FD list. Aborting", get_timestamp());
      exit (2);
    }

    numEvents = epoll_wait (epollFD, &processableEvents, 1, -1);

    if (numEvents < 0) {
      printf ("\nsckt_client - ERROR - Serious error in epoll setup: epoll_wait () returned < 0 status!");
      fprintf(main_log_fd, "\n%s - ERROR - sckt_client - Serious error in epoll setup: epoll_wait () returned < 0 status", get_timestamp());
      exit (2);
    }

    if (getsockopt (client_sockfd, SOL_SOCKET, SO_ERROR, &retVal, &retValLen) < 0) {
      // ERROR, fail somehow, close socket
      printf ("\nsckt_client - ERROR - fail somehow, close socket!");
      fprintf(main_log_fd, "\n%s - ERROR - sckt_client - fail somehow, close socket", get_timestamp());
    }

    if (retVal != 0) {
      // ERROR: connect did not "go through"
      printf ("\nsckt_client - ERROR - connect did not go through!");
      fprintf(main_log_fd, "\n%s - ERROR - sckt_client - connect did not go through", get_timestamp());
    }   
  }
  else {
    // ERROR: connect did not "go through" for other non-recoverable reasons.
    printf ("\ndckt_client - ERROR - Connect did not go through for other non-recoverable reasons!");
    fprintf(main_log_fd, "\n%s - ERROR - sckt_client - connect did not go through for other non-recoverable reasons", get_timestamp());
  }

  Connected = true;
  }
  // Connect Block Ends Here
  // ///////////////////////////////////

  if (verbose)
    printf("\nsckt_client - connected");
  fprintf(main_log_fd, "\n%s - INFO - sckt_client - connected", get_timestamp());


  // ///////////////////////////////////
  // Client Block Starts Here
  struct epoll_event newPeerConnectionEvent;
  struct epoll_event processableEvents[MAX_EPOLL_EVENTS];
  
  int EPFD = epoll_create(5);

  if (EPFD == -1) {
    printf("\nsckt_client - ERROR - epoll_create failed, client thread exiting");
    fprintf(main_log_fd, "\n%s - ERROR - sckt_client - epoll_create failed, client thread exiting", get_timestamp());
    Connected = false;
    return;
  }

  #ifndef EPOLLRDHUP
  #define EPOLLRDHUP 0x2000
  #endif

  newPeerConnectionEvent.data.fd = client_sockfd;
  newPeerConnectionEvent.events = EPOLLIN | EPOLLRDHUP | EPOLLET; // | EPOLLOUT

  if (epoll_ctl(EPFD, EPOLL_CTL_ADD, client_sockfd, &newPeerConnectionEvent) == -1) {
    printf("\nsckt_client - ERROR - epoll_ctl_add failed, client thread exiting");
    fprintf(main_log_fd, "\n%s - ERROR - sckt_client - epoll_ctl_add failed, client thread exiting", get_timestamp());
    Connected = false;
    return;
  }

  // now wait for data Rx events
  while (Connected) {
    int cnt = epoll_wait(EPFD, &processableEvents, MAXEVENTS, -1);

    if (cnt == -1 && errno != EINTR) {
      printf("\nsckt_client - ERROR - epoll fault");
      fprintf(main_log_fd, "\n%s - ERROR - sckt_client - epoll fault", get_timestamp());
      Connected = false;
    }
    else if(cnt) {
      uint32_t evt = processableEvents[0].events;
      if( evt & EPOLLRDHUP ) {
        // remote shutdown.
        Connected = false;
        if (verbose)
          printf("\nsckt_client - remote connection went away");
        fprintf(main_log_fd, "\n%s - INFO - sckt_client - remote connection went away", get_timestamp());
      }
      else if(evt & EPOLLIN) {
        if (verbose)
          printf("\nsckt_client - Client socket epoll RX triggered!");
      }
      else if(evt & EPOLLOUT) {
        if (verbose)
          printf("\nsckt_client - Client socket epoll TX triggered!");
//        write(client_sockfd, ..., ...); 
      }
      else //if( (evt & EPOLLHUP) || (evt & EPOLLERR) )
        printf("\nsckt_client - ERROR - clientproc unhandled event");
    }

  }
  // Client Block Ends Here
  // ///////////////////////////////////

  close(client_sockfd);

  fprintf(main_log_fd, "\n%s - INFO - sckt_client - Client exiting", get_timestamp());
  if (verbose)
    printf("\nsckt_client - scktClientProc exiting");
}


// Interface function to xmit data
uint32_t ipc_xmit (uint8_t *buf, int32_t bufSize)
{
  unsigned int totBytesWritten = 0;
  int numWritten = 0;
  int i;

  if (verbose)
    printf("\nsckt_server - ipc_xmit\n");
  fprintf(main_log_fd, "\n%s - INFO - sckt_client - ipc_xmit", get_timestamp());

  if (!Connected || client_sockfd <= 0) {
    if (verbose)
      printf("\nsckt_server - ipc_xmit Client Not Connected!\n");
    fprintf(main_log_fd, "\n%s - INFO - sckt_client - ipc_xmit - Client Not Connected!", get_timestamp());
    return 0;
  }

  numWritten = send(client_sockfd, buf, bufSize, 0);
  if (numWritten <= 0) {
    if (numWritten == -1 && errno != EINTR) {
      system_error("sckt_client - ipc_xmit - socket write error, aborting send");
      return 0;  // some error
    }
    system_error("sckt_client - ipc_xmit - socket write error, num of written is less than zero");
  }
  else
    totBytesWritten += numWritten;

  numWritten = 0;

  fprintf(main_log_fd, "\n%s - INFO - sckt_client - sent = %d bytes", get_timestamp(), totBytesWritten);

  fprintf(log_fd, "\n%s - INFO - sckt_client - ", get_timestamp());

#if 1
//    printf("Shared memory contains: \"%s\"\n", prod_shm);
   for (i = 0; i < bufSize; i++) {
     fprintf(log_fd, "0x%X,", buf[i] & 0x000000FF);
   }
#endif

  return totBytesWritten;
}


// Interface function to receive data
void ipc_rec (uint8_t *buf, int32_t bufSize)
{
  system_error ("sckt_client - ipc_rec - Not implemented");
}
