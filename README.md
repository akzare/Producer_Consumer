# Consumer/Producer design project

This repository presents a conceptual software design for creating and verifying a typical producer/consumer system on multiple embedded processors. It includes these features:

 * The program incorporates a conceptual Inter SoC Communication (ISC) system. Local or remote processes exchange information by transmitting/receiving bulk data.
 * The program handles the transmission and reception of data by means of an efficient C program which utilizes multi-threading.
 * For data exchange among processes on local and remote machines, different transport mechanisms in the form of loadable dynamically shared libraries (modules) are implemented. 
 * Shared memory transport module is used as a very low latency media for data exchange among different processes on a local machine.
 * Linux epoll based socket transport is utilized for data exchange among different processes on remote machines. 
 * Each subsystem generates log files which include formatted timestamped events for later analysis.
 * In order to automatically copy the system from host machine to remote embedded machines, a utility Python script is provided. This script is used to copy, build, and run the application on multiple embedded boards. The generated log files on remote machines can be copied back to host machine by using this script. 
 * Another analysis Python script is implemented to parse the event elements in the log files. It can draw the timing flow diagram of the events. 
 

# General architecture

The system design architecture includes these subsystems:
 * Producer subsystem: It produces a chunk of data in a shared memory which is shared with the ISC Client subsystem. The shared memory between these two processes is synchronized by using binary semaphore.
 * ISC Client Subsystem: It implements two transports: i- Receiver Shared Memory, ii- TCP Client. The shared memory transport is being used to get data from Producer subsystem and in case of receiving each chunk of data, it invokes a callback in the TCP client transport module. The TCP client transport is based on epoll in Linux and it connects to the ICS Server on another remote machine.
 * ISC Server Subsystem: It implements two transports: i- TCP Server, ii- Transmitter Shared Memory.  The TCP server transport is based on epoll in Linux and the TCP client subsystems on any number of nodes can connect to this ICS Server. As TCP server receives each new chunk of data, it invokes the transmitter callback in the transmitter shared memory module.
 * Consumer subsystem: It receives the provided data by the ISC Server subsystem over shared memory.


# Building the ISC system manually

In order to manually build the system, you should first setup the cross-compiler on you host machine. Provided that the target embedded board has GCC compiler on its Linux distribution, as an alternative you might use the target embedded platform to build the code. Building of the program manually is easy. First copy the C source code into board and then from the directory containing the sources, 
simply invoke make:
  % make
  % make prod
  % make cons

This builds the isc program and the isc module shared libraries.


# Building the ISC system automatically

To build and run the isc, two embedded boards which are connected over a network switch to the host PC can be used. First some settings such as boards IP addresses and the location of the code on the host and remote machines must be configured in Config.py script. Afterwards, the prepared copy, build, and run python script (CopyBuildRun.py) should be used. This script simplifies and automates the procedure and runs a bunch of commands remotely from your host PC on the target embedded boards.

  % python3 CopyBuildRun.py

The resulted log files are automatically copied to the log directory on the host machine. The log files represent the behavior of each subsystem (process or thread) with timestamped events. 


# Parsing and Analyzing the log files

In order to parse and extract the subsystem's data flow out of the log files, the python AnlyzLogFiles.py can be utilized. This Python script utilizes a customized parser class (Log_File_Parser) to parse each line of the log files into meaningful data structures. It discovers the occurred errors, and warnings in each log file and reflect them in its output result file (report_dataflow.log). Furthermore, this report file creates a "Data-flow sequence" table which clearly represents the series of happened events in the system in a sorted time based manner. It greatly helps to understand the system data flow in an easy way. Moreover, it generates a graph out of this analyzed data which helps to
visualize this table in the form of a graph. The "Payload match" section in this file, shows the linked correlated events on different subsystems. It helps to discover the data flow sequence between different subsystems.


# Contributing

If you'd like to add or improve this software design, your contribution is welcome!


# License

This repository is released under the [MIT license](https://opensource.org/licenses/MIT). In short, this means you are free to use this software in any personal, open-source or commercial projects. Attribution is optional but appreciated.
