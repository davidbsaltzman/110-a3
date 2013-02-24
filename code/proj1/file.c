#include <stdio.h>
#include <stdlib.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"

/*
 * Fetch the specified file block from the specified inode.
 * Return the number of valid bytes in the block, -1 on error.
 */
int
file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf)
{
  struct inode in;
  if (inode_iget(fs, inumber, &in) < 0) {
    fprintf(stderr,"Can't read inode %d \n", inumber);
    return -1;
  }
  
  int blockAddress = inode_indexlookup(fs, &in, blockNum);
  
  if (blockAddress == -1) {
    fprintf(stderr, "Error looking up block number %d in inode %d", blockNum, inumber);
    return -1;
  }
  
  int size = inode_getsize(&in);
  int bytesInBlock = size > ((blockNum + 1) * DISKIMG_SECTOR_SIZE) ? 512 :
                            size - (blockNum * DISKIMG_SECTOR_SIZE);
  
  if (diskimg_readsector(fs->dfd, blockAddress, buf) != DISKIMG_SECTOR_SIZE) {
    fprintf(stderr, "Error reading block contents for block %d in inode %d\n", blockNum, inumber);
    return -1;
  }
  
  return bytesInBlock;
}
