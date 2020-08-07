# * @file   Makefile
# * @author The main body of this code is from Advanced Linux Programming book by 
# *         Mark Mitchell, Jeffrey Oldham, and Alex Samuel.
# *         Further modification of code is done by Armin Zare Zadeh.
# * @date   04 August 2020
# * @version 0.1
# * @brief   The Makefile provides these targets:
# * - all (the default if you invoke make without arguments because it's the first target
# *   in the Makefile) includes the isc executable and all the modules.The modules are 
# *   listed in the variable MODULES.
# * 
# * - clean deletes any build products that are produced by the Makefile.
# * 
# * - isc links the isc executable.The source files listed in the variable SOURCES
# *   are compiled and linked in.
# * 
# * - The last rule is a generic pattern for compiling shared object files for isc
# *   modules from the corresponding source files.
# * 
# * Note that source files for isc modules are compiled with the -fPIC option because
# * they are linked into shared libraries.
# * Also observe that the isc executable is linked with the -Wl,-export-dynamic
# * compiler option. With this option, GCC passes the -export-dynamic option to the
# * linker, which creates an executable file that also exports its external symbols as a shared
# * library.This allows modules, which are dynamically loaded as shared libraries, to reference 
# * functions from common.c that are linked statically into the isc executable.


### Configuration. ####################################################

# Default C compiler options.
CFLAGS = -Wall -g
# C source files for the isc.
SOURCES = isc.c ipc.c common.c main.c
# Corresponding object files.
OBJECTS = $(SOURCES:.c=.o)
# ipc module shared library files.
MODULES = shmem_rec.so shmem_xmit.so sckt_server.so sckt_client.so

### Rules. ############################################################

# Phony targets don't correspond to files that are built; they're names
# for conceptual build targets.
.PHONY: all clean

# Default target: build everything.
all: isc $(MODULES)

# Clean up build products.
clean:
	rm -f $(OBJECTS) $(MODULES) isc

# Build producer.
prod: producer.c common.c isc.h
	cc -o producer producer.c common.c -lpthread

# Clean up producer.
clean_prod:
	rm -f producer

# Build consumer.
cons: consumer.c common.c isc.h
	cc -o consumer consumer.c common.c -lpthread

# Clean up consumer.
clean_cons:
	rm -f consumer

# The main isc program. Link with -Wl,-export-dyanamic so
# dynamically loaded modules can bind symbols in the program. Link in
# libdl, which contains calls for dynamic loading.
isc: $(OBJECTS)
	$(CC) $(CFLAGS) -Wl,-export-dynamic -o $@ $^ -ldl -lpthread

# All object files in the isc depend on isc.h. But use the
# default rule for building object files from source files.
$(OBJECTS): isc.h

# Rule for building module shared libraries from the corresponding
# source files. Compile -fPIC and generate a shared object file.
$(MODULES): \
%.so: %.c isc.h
	$(CC) $(CFLAGS) -fPIC -shared -o $@ $<
