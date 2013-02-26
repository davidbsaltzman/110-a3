#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "file.h"
#include "inode.h"
#include "diskimg.h"
#include "../debug.h"

int storediNum = 0;
int storedBlockNum;
char storedBlock[DISKIMG_SECTOR_SIZE];
int storedBytes;

/*
 * Fetch the specified file block from the specified inode.
 * Return the number of valid bytes in the block, -1 on error.
 */
int
file_getblock(struct unixfilesystem *fs, int inumber, int blockNum, void *buf)
{
  // if the block is previously stored, return it immediately
  if ( (inumber == storediNum) && (blockNum == storedBlockNum) )
  {
    memcpy(buf, storedBlock, DISKIMG_SECTOR_SIZE);
    return storedBytes;
  }
  DPRINTF('b',("inumber - block: %d - %d\n", inumber, blockNum));
  
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
  
  // store the new block
  storediNum = inumber;
  storedBlockNum = blockNum;
  memcpy(storedBlock, buf, DISKIMG_SECTOR_SIZE);
  storedBytes = bytesInBlock;
  
  return bytesInBlock;
}
