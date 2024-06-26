/*************************************************
*     Exim - an Internet mail transport agent    *
*************************************************/

/* Copyright (c) University of Cambridge 1995 - 2009 */
/* See the file NOTICE for conditions of use and distribution. */


/* This is the only Exim header that you should include. The effect of
including any other Exim header is not defined, and may change from release to
release. Use only the documented interface! */

#include "local_scan.h"

#ifdef DLOPEN_LOCAL_SCAN
#include <dlfcn.h>
static int (*local_scan_fn)(int fd, uschar **return_text) = NULL;
static int load_local_scan_library(void);
#endif

int
local_scan(int fd, uschar **return_text)
{

#ifdef DLOPEN_LOCAL_SCAN
/* local_scan_path is defined AND not the empty string */
if (local_scan_path && *local_scan_path)
  {
  if (!local_scan_fn)
    {
    if (!load_local_scan_library())
      {
        char *base_msg , *error_msg , *final_msg ;
        int final_length = -1 ;

        base_msg=US"Local configuration error - local_scan() library failure\n";
        error_msg = dlerror() ;

        final_length = strlen(base_msg) + strlen(error_msg) + 1 ;
        final_msg = (char*)malloc( final_length*sizeof(char) ) ;
        *final_msg = '\0' ;

        strcat( final_msg , base_msg ) ;
        strcat( final_msg , error_msg ) ;

        *return_text = final_msg ;
      return LOCAL_SCAN_TEMPREJECT;
      }
    }
    return local_scan_fn(fd, return_text);
  }
else
#endif
  return LOCAL_SCAN_ACCEPT;
}

#ifdef DLOPEN_LOCAL_SCAN

static int load_local_scan_library(void)
{
/* No point in keeping local_scan_lib since we'll never dlclose() anyway */
void *local_scan_lib = NULL;
int (*local_scan_version_fn)(void);
int vers_maj;
int vers_min;

local_scan_lib = dlopen(local_scan_path, RTLD_NOW);
if (!local_scan_lib)
  {
  log_write(0, LOG_MAIN|LOG_REJECT, "local_scan() library open failed - "
    "message temporarily rejected");
  return FALSE;
  }

local_scan_version_fn = dlsym(local_scan_lib, "local_scan_version_major");
if (!local_scan_version_fn)
  {
  dlclose(local_scan_lib);
  log_write(0, LOG_MAIN|LOG_REJECT, "local_scan() library doesn't contain "
    "local_scan_version_major() function - message temporarily rejected");
  return FALSE;
  }

/* The major number is increased when the ABI is changed in a non
   backward compatible way. */
vers_maj = local_scan_version_fn();

local_scan_version_fn = dlsym(local_scan_lib, "local_scan_version_minor");
if (!local_scan_version_fn)
  {
  dlclose(local_scan_lib);
  log_write(0, LOG_MAIN|LOG_REJECT, "local_scan() library doesn't contain "
    "local_scan_version_minor() function - message temporarily rejected");
  return FALSE;
  }

/* The minor number is increased each time a new feature is added (in a
   way that doesn't break backward compatibility) -- Marc */
vers_min = local_scan_version_fn();


if (vers_maj != LOCAL_SCAN_ABI_VERSION_MAJOR)
  {
  dlclose(local_scan_lib);
  local_scan_lib = NULL;
  log_write(0, LOG_MAIN|LOG_REJECT, "local_scan() has an incompatible major"
    "version number, you need to recompile your module for this version"
    "of exim (The module was compiled for version %d.%d and this exim provides"
    "ABI version %d.%d)", vers_maj, vers_min, LOCAL_SCAN_ABI_VERSION_MAJOR,
    LOCAL_SCAN_ABI_VERSION_MINOR);
  return FALSE;
  }
else if (vers_min > LOCAL_SCAN_ABI_VERSION_MINOR)
  {
  dlclose(local_scan_lib);
  local_scan_lib = NULL;
  log_write(0, LOG_MAIN|LOG_REJECT, "local_scan() has an incompatible minor"
    "version number, you need to recompile your module for this version"
    "of exim (The module was compiled for version %d.%d and this exim provides"
    "ABI version %d.%d)", vers_maj, vers_min, LOCAL_SCAN_ABI_VERSION_MAJOR,
    LOCAL_SCAN_ABI_VERSION_MINOR);
  return FALSE;
  }

local_scan_fn = dlsym(local_scan_lib, "local_scan");
if (!local_scan_fn)
  {
  dlclose(local_scan_lib);
  log_write(0, LOG_MAIN|LOG_REJECT, "local_scan() library doesn't contain "
    "local_scan() function - message temporarily rejected");
  return FALSE;
  }
return TRUE;
}

#endif /* DLOPEN_LOCAL_SCAN */

/* End of local_scan.c */
