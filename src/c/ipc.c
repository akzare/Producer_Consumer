/**
 * @file   ipc.c
 * @author The main body of this code is from Advanced Linux Programming book by 
 *         Mark Mitchell, Jeffrey Oldham, and Alex Samuel.
 *         Further modification of code is done by Armin Zare Zadeh
 * @date   04 August 2020
 * @version 0.1
 * @brief   module.c provides the implementation of dynamically loadable
 *          Inter Process Communication (IPC) modules. A loaded IPC module is represented 
 *          by an instance of struct ipc_module, which is defined in isc.h.
 *          Each module is a shared library file and must define and export a function named 
 *          module_generate.
 * 
 * ipc.c contains two functions:
 * 
 * - ipc_open attempts to load an ISC module with a given name. The name
 *   normally ends with the .so extension because ISC modules are implemented
 *   as shared libraries. This function opens the shared library with dlopen and
 *   resolves a symbol named module_generate from the library with dlsym. If the library
 *   can't be opened, or if module_generate isn't a name exported by the library, the
 *   call fails and ipc_open returns a null pointer. Otherwise, it allocates and
 *   returns a module object.
 * 
 * - ipc_close closes the shared library corresponding to the ISC module and
 *   deallocates the struct ipc_module object.
 *
 *   ipc.c also defines a global variable module_dir. This is the path of the directory in
 *   which ipc_open attempts to find shared libraries corresponding to server modules.
 */

#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "isc.h"


/*********************************************************************************** 
 * C o n s t a n t s ,   v a r i a b l e s ,  f u n c t i o n s 
************************************************************************************/

// The directory of module shared libraries on file system
char* module_dir;


// The interface function to open an IPC loadable shared library at run-time
struct ipc_module* ipc_open (const char* module_name)
{
  char* module_path;
  void* handle;
  void (* ipc_init) (void (*ipc_rec)(unsigned char *buf, int bufSize), void (*ipc_xmit)(uint8_t *buf, int32_t bufSize));
  void (* ipc_cleanup) ();
  void (* ipc_stop) ();
  bool (* ipc_start) ();
  bool (* ipc_wait4Done) ();
  bool (* ipc_set_param) (const char* prtcl, const char *addr, int port);
  uint32_t (* ipc_xmit) (uint8_t *buf, int32_t bufSize);
  void (* ipc_rec) (uint8_t *buf, int32_t bufSize);
  struct ipc_module* module;

  // Construct the full path of the module shared library we'll try to
  // load.
  module_path = (char*) xmalloc (strlen (module_dir) + strlen (module_name) + 2);
  sprintf (module_path, "%s/%s", module_dir, module_name);

  // Attempt to open MODULE_PATH as a shared library.
  handle = dlopen (module_path, RTLD_NOW);
  free (module_path);
  if (handle == NULL) {
    // Failed; either this path doesn't exist or it isn't a shared
    // library.
    return NULL;
  }

  // Resolve the ipc_init symbol from the shared library.
  ipc_init = (void (*) (void (*ipc_rec)(uint8_t *buf, int32_t bufSize), void (*ipc_xmit)(uint8_t *buf, int32_t bufSize))) dlsym (handle, "ipc_init");
  // Make sure the symbol was found.
  if (ipc_init == NULL) {
    // The symbol is missing. While this is a shared library, it
    // probably isn't a server module. Close up and indicate failure.
    dlclose (handle);
    return NULL;
  }

  // Resolve the ipc_cleanup symbol from the shared library.
  ipc_cleanup = (void (*) ()) dlsym (handle, "ipc_cleanup");
  // Make sure the symbol was found.
  if (ipc_cleanup == NULL) {
    // The symbol is missing. While this is a shared library, it
    // probably isn't a server module. Close up and indicate failure.
    dlclose (handle);
    return NULL;
  }

  // Resolve the ipc_stop symbol from the shared library.
  ipc_stop = (void (*) ()) dlsym (handle, "ipc_stop");
  // Make sure the symbol was found.
  if (ipc_stop == NULL) {
    // The symbol is missing. While this is a shared library, it
    // probably isn't a server module. Close up and indicate failure.
    dlclose (handle);
    return NULL;
  }
  

  // Resolve the ipc_start symbol from the shared library.
  ipc_start = (bool (*) ()) dlsym (handle, "ipc_start");
  // Make sure the symbol was found.
  if (ipc_start == NULL) {
    // The symbol is missing. While this is a shared library, it
    // probably isn't a server module. Close up and indicate failure.
    dlclose (handle);
    return NULL;
  }

  // Resolve the ipc_wait4Done symbol from the shared library.
  ipc_wait4Done = (bool (*) ()) dlsym (handle, "ipc_wait4Done");
  // Make sure the symbol was found.
  if (ipc_wait4Done == NULL) {
    // The symbol is missing. While this is a shared library, it
    // probably isn't a server module. Close up and indicate failure.
    dlclose (handle);
    return NULL;
  }

  // Resolve the ipc_set_param symbol from the shared library.
  ipc_set_param = (bool (*) (const char* prtcl, const char *hostname, int port)) dlsym (handle, "ipc_set_param");
  // Make sure the symbol was found.
  if (ipc_set_param == NULL) {
    // The symbol is missing. While this is a shared library, it
    // probably isn't a server module. Close up and indicate failure.
    dlclose (handle);
    return NULL;
  }

  // Resolve the ipc_xmit symbol from the shared library.
  ipc_xmit = (uint32_t (*) (uint8_t *buf, int32_t bufSize)) dlsym (handle, "ipc_xmit");
  // Make sure the symbol was found.
  if (ipc_xmit == NULL) {
    // The symbol is missing. While this is a shared library, it
    // probably isn't a server module. Close up and indicate failure.
    dlclose (handle);
    return NULL;
  }

  // Resolve the ipc_rec symbol from the shared library.
  ipc_rec = (void (*) (uint8_t *buf, int32_t bufSize)) dlsym (handle, "ipc_rec");
  // Make sure the symbol was found.
  if (ipc_rec == NULL) {
    // The symbol is missing. While this is a shared library, it
    // probably isn't a server module. Close up and indicate failure.
    dlclose (handle);
    return NULL;
  }

  // Allocate and initialize a ipc_module object.
  module = (struct ipc_module*) xmalloc (sizeof (struct ipc_module));
  module->handle = handle;
  module->name = xstrdup (module_name);
  module->init_function = ipc_init;
  module->cleanup_function = ipc_cleanup;
  module->stop_function = ipc_stop;
  module->start_function = ipc_start;
  module->wait4Done_function = ipc_wait4Done;
  module->set_param_function = ipc_set_param;
  module->xmit_function = ipc_xmit;
  module->rec_function = ipc_rec;

  // Return it, indicating success.
  return module;
}


// The interface function to close an already opened IPC loadable shared library
void ipc_close (struct ipc_module* module)
{
  // Close the shared library.
  dlclose (module->handle);

  // Deallocate the module name.
  free ((char*) module->name);

  // Deallocate the module object.
  free (module);
}
