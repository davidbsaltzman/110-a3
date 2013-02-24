#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "remove.h"
#include "inode.h"
#include "diskimg.h"

int remove_file(struct unixfilesystem *fs, int inumber)
{
  struct inode in;
  int err = inode_iget(fs, inumber, &in);
  if (err < 0) return -1;
  
  if (!(in.i_mode & IALLOC) || ((in.i_mode & IFMT) == IFDIR))
  {
    /* Not allocated or is a directory */
    fprintf(stderr, "The input path is not a file and cannot be removed");
    return -1;
  }
  
  if (in.i_nlink == 1) // only one link, so overwrite the file
  {
    char zeros[DISKIMG_SECTOR_SIZE] = {0};
    int size = inode_getsize(&in);
    int blockNum = 0;
    
    // overwrite the file with zeros
    for (blockNum = 0; blockNum < size / DISKIMG_SECTOR_SIZE; blockNum++)
    {
      char buf[DISKIMG_SECTOR_SIZE];
      
      int blockAddress = inode_indexlookup(fs, &in, blockNum);
      
      if (blockAddress == -1) {
        fprintf(stderr, "Error looking up block number %d in inode %d", blockNum, inumber);
        return -1;
      }
      
      if (diskimg_writesector(fs->dfd, blockAddress, &zeros) == -1)
      {
        fprintf(stderr, "Error overwriting file");
        return -1;
      }
    }
  }
  else  // multiple links to file, so only remove this link
  {
    in.i_nlink--;
    
  }
  
  return 0;
}