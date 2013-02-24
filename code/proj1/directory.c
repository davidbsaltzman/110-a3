
#include "directory.h"
#include "inode.h"
#include "diskimg.h"
#include "file.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>


/*
 * Lookup the specified name (name) in the directory (dirinumber). If found, return the 
 * directory entry in dirEnt. Return 0 on success and something negative on failure. 
 */
int
directory_findname(struct unixfilesystem *fs, const char *name,
                   int dirinumber, struct direntv6 *dirEnt)
{
  struct inode in;
  int err = inode_iget(fs, dirinumber, &in);
  if (err < 0) return err;
  
  if (!(in.i_mode & IALLOC) || ((in.i_mode & IFMT) != IFDIR)) {
    fprintf(stderr, "Path %s in not allocated or not a directory\n", name);
    return -1;
  }
  
  int size = inode_getsize(&in);
  
  assert((size % sizeof(struct direntv6)) == 0);
  
  int count = 0;
  int numBlocks  = (size + DISKIMG_SECTOR_SIZE - 1) / DISKIMG_SECTOR_SIZE;
  
  char buf[DISKIMG_SECTOR_SIZE];
  struct direntv6 *dir = (struct direntv6 *) buf;
  
  for (int bno = 0; bno < numBlocks; bno++) {
    int bytesInBlock, numEntriesInBlock, i;
    bytesInBlock = file_getblock(fs, dirinumber,bno,dir);
    if (bytesInBlock < 0) {
      fprintf(stderr, "Error reading directory %s\n", name);
      return -1;
    }
    
    numEntriesInBlock = bytesInBlock/sizeof(struct direntv6);
    for (i = 0; i < numEntriesInBlock; i++) {
      if (strcmp(dir[i].d_name, name) == 0)
      {
        strcpy(dirEnt->d_name, name);
        dirEnt->d_inumber = dir[i].d_inumber;
        return 0;
      }
    }
  }
  return -1;
}
