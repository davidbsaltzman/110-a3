#include "pathname.h"
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>

/*
 * Return the inumber associated with the specified pathname. This need only
 * handle absolute paths. Return a negative number if an error is encountered.
 */
int
pathname_lookup(struct unixfilesystem *fs, const char *pathname)
{
  char done = 0;
  
  const char *currentPath = pathname + 1; // throw out the initial '/' character
  
  struct direntv6 currentDir = {ROOT_INUMBER, {'\0'}};
    
  while (!done)         // each loop goes one directory deeper until reaching the end of the pathname
  {
    char d_name[14];
    
    const char *slashPoint = strchr(currentPath, '/');        // pointer to the next '/' in the currentpath
    
    if (slashPoint)                                           // there is a remaining '/'; ie, there are deeper levels to go into
    {
      strncpy(d_name, currentPath, slashPoint - currentPath); // copy into d_name just the part of the current path before the first '/'
      d_name[slashPoint - currentPath] = '\0';                // null terminated
      currentPath = slashPoint + 1;                           // throw out the '/'
    }
    else // the currentpath is the file being looked up
    {
      strcpy(d_name, currentPath);
      done = 1;
    }
    
    directory_findname(fs, d_name, currentDir.d_inumber, &currentDir); // replace currentDir with the next subdirectory or the final file
  }

  return currentDir.d_inumber;
}
