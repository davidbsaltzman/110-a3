#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include "inode.h"
#include "diskimg.h"
#include "../debug.h"

#define BLOCK_ADDRESSES_PER_SECTOR (DISKIMG_SECTOR_SIZE / 2)

int stored_inode_number = -1;
struct inode stored_inode;

/*
 * Fetch the specified inode from the filesystem. 
 * Return 0 on success, -1 on error.  
 */
int
inode_iget(struct unixfilesystem *fs, int inumber, struct inode *inp)
{
  if (inumber == stored_inode_number)
  {
    *inp = stored_inode;
    return 0;
  }
  
  struct inode inodeSector[16];
  
  for (int i = 0; i < fs->superblock.s_ninode; i++)
  {
    if (fs->superblock.s_inode[i] == inumber)
      return -1;
  }
  
  if (diskimg_readsector(fs->dfd, INODE_START_SECTOR + ((inumber - 1) / 16), &inodeSector) != DISKIMG_SECTOR_SIZE) {
      fprintf(stderr, "Error reading inode %d\n", inumber);
      return -1;
  }
  
  *inp = inodeSector[(inumber - 1) % 16];
  
  stored_inode_number = inumber;
  stored_inode = *inp;
  
  return 0;
  
}

int storedIndexSector1Num = -1;
int storedIndexSector2Num = -1;
uint16_t storedIndexSector1[BLOCK_ADDRESSES_PER_SECTOR];
uint16_t storedIndexSector2[BLOCK_ADDRESSES_PER_SECTOR];

/*
 * Get the location of the specified file block of the specified inode.
 * Return the disk block number on success, -1 on error.  
 */
int
inode_indexlookup(struct unixfilesystem *fs, struct inode *inp, int blockNum)
{
  int blockAddress = -1;
  int numBlocks = inode_getsize(inp) / DISKIMG_SECTOR_SIZE;
  
  if (numBlocks < 8)
  {
    if (blockNum < 8)
      blockAddress = inp->i_addr[blockNum];
    else
    {
      fprintf(stderr, "Block number out of range");
      return -1;
    }
  }
  else // use indirect or doubly indirect addressing
  {
    int indirectBlockNum = blockNum / BLOCK_ADDRESSES_PER_SECTOR;
    
    if (indirectBlockNum < 7) // singly indirect
    {
      if (inp->i_addr[indirectBlockNum] == storedIndexSector1Num)
      {DPRINTF('x',("single prestored: %d -> %d (%d)\n", storedIndexSector1Num, inp->i_addr[indirectBlockNum],blockNum));
        blockAddress = storedIndexSector1[blockNum - (indirectBlockNum * BLOCK_ADDRESSES_PER_SECTOR)];
      }
      else
      {DPRINTF('x',("single fetch %d -> %d\n", storedIndexSector1Num, inp->i_addr[indirectBlockNum]));
        uint16_t fetchedBlock[BLOCK_ADDRESSES_PER_SECTOR];
        
        if (diskimg_readsector(fs->dfd, inp->i_addr[indirectBlockNum], &fetchedBlock) != DISKIMG_SECTOR_SIZE) {
          fprintf(stderr, "Error reading indirect block\n");
          return -1;
        }
        
        blockAddress = fetchedBlock[blockNum - (indirectBlockNum * BLOCK_ADDRESSES_PER_SECTOR)];
        
        storedIndexSector1Num = inp->i_addr[indirectBlockNum];
        memcpy(storedIndexSector1, fetchedBlock, DISKIMG_SECTOR_SIZE);
      }
    }
    else // doubly indirect
    {
      int positionInFirstBlock = (blockNum - (7 * BLOCK_ADDRESSES_PER_SECTOR)) / BLOCK_ADDRESSES_PER_SECTOR;
      int secondBlockAddress;
      
      if (inp->i_addr[7] == storedIndexSector1Num)
      {DPRINTF('x',("double prestored - 1\n"));
        secondBlockAddress = storedIndexSector1[positionInFirstBlock];
      }
      else
      {DPRINTF('x',("double fetch - 1\n"));
        uint16_t firstFetchedBlock[BLOCK_ADDRESSES_PER_SECTOR];
        
        if (diskimg_readsector(fs->dfd, inp->i_addr[7], &firstFetchedBlock) != DISKIMG_SECTOR_SIZE) {
          fprintf(stderr, "Error reading first doubly indirect block\n");
          return -1;
        }
        
        secondBlockAddress = firstFetchedBlock[positionInFirstBlock];
        
        storedIndexSector1Num = inp->i_addr[7];
        memcpy(storedIndexSector1, firstFetchedBlock, DISKIMG_SECTOR_SIZE);
      }
      
      int positionInSecondBlock = blockNum - (7 * BLOCK_ADDRESSES_PER_SECTOR) - positionInFirstBlock * BLOCK_ADDRESSES_PER_SECTOR;
      
      if (secondBlockAddress == storedIndexSector2Num)
      {DPRINTF('x',("double prestored - 2\n"));
        blockAddress = storedIndexSector2[positionInSecondBlock];
      }
      else
      {DPRINTF('x',("double fetch - 2\n"));
        uint16_t secondFetchedBlock[BLOCK_ADDRESSES_PER_SECTOR];
        
        if (diskimg_readsector(fs->dfd, secondBlockAddress, &secondFetchedBlock) != DISKIMG_SECTOR_SIZE) {
          fprintf(stderr, "Error reading second doubly indirect block\n");
          return -1;
        }
       
        blockAddress = secondFetchedBlock[positionInSecondBlock];
        
        storedIndexSector2Num = secondBlockAddress;
        memcpy(storedIndexSector2, secondFetchedBlock, DISKIMG_SECTOR_SIZE);
      }
    }
  }
  
  return blockAddress;
}

/* 
 * Compute the size of an inode from its size0 and size1 fields.
 */
int
inode_getsize(struct inode *inp) 
{
  return ((inp->i_size0 << 16) | inp->i_size1); 
}
