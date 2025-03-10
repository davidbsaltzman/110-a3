/*
 * pathstore.c  - Store pathnames for indexing
 */

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <assert.h>
#include <ctype.h>
#include <string.h>
#include <inttypes.h>

#include "index.h"
#include "fileops.h"
#include "pathstore.h"
#include "proj1/chksumfile.h"

typedef struct PathstoreElement {
  char *pathname;
  struct PathstoreElement *nextElement;
  char chksum[CHKSUMFILE_SIZE];
} PathstoreElement;

char currentChksum[CHKSUMFILE_SIZE];  // keep track of the chksum of the path being added
int currentChksumComputed = 0;        // only calculate it if needed, and not more than once

static uint64_t numdifferentfiles = 0;
static uint64_t numsamefiles = 0;
static uint64_t numdiffchecksum = 0;
static uint64_t numdups = 0;
static uint64_t numcompares = 0;
static uint64_t numstores = 0;

static int SameFileIsInStore(Pathstore *store, char *pathname);
static int IsSameFile(Pathstore *store, char *pathname1, PathstoreElement *e);

Pathstore*
Pathstore_create(void *fshandle)
{
  Pathstore *store = malloc(sizeof(Pathstore));
  if (store == NULL)
    return NULL;

  store->elementList = NULL;
  store->fshandle = fshandle;

  return store;
}

/*
 * Free up all the sources allocated for a pathstore.
 */
void
Pathstore_destroy(Pathstore *store)
{
  PathstoreElement *e = store->elementList;

  while (e) {
    PathstoreElement *next = e->nextElement;
    free(e->pathname);
    free(e);
    e = next;
  }
  free(store);
}

/*
 * Store a pathname in the pathname store.
 */
char*
Pathstore_path(Pathstore *store, char *pathname, int discardDuplicateFiles)
{
  PathstoreElement *e;

  numstores++;

  if (discardDuplicateFiles) {
    if (SameFileIsInStore(store,pathname)) {
      numdups++;
      return NULL;
    }
  }

  e = malloc(sizeof(PathstoreElement));
  if (e == NULL) {
    return NULL;
  }

  e->pathname = strdup(pathname);
  if (e->pathname == NULL) {
    free(e);
    return NULL;
  }
  e->nextElement = store->elementList;
  store->elementList = e;

  if (currentChksumComputed) {
    memcpy(e->chksum, currentChksum, CHKSUMFILE_SIZE);
  }

  return e->pathname;

}

/*
 * Is this file the same as any other one in the store
 */
static int
SameFileIsInStore(Pathstore *store, char *pathname)
{
  currentChksumComputed = 0;
  
  PathstoreElement *e = store->elementList;

  while (e) {
    if (IsSameFile(store, pathname, e)) {
      return 1;  // In store already
    }
    e = e->nextElement;
  }
  return 0; // Not found in store
}

/*
 * Do the two pathnames refer to a file with the same contents.
 */
static int
IsSameFile(Pathstore *store, char *pathname1, PathstoreElement *e)
{
  struct unixfilesystem *fs = (struct unixfilesystem *) (store->fshandle);

  numcompares++;
  if (strcmp(pathname1, e->pathname) == 0) {
    return 1; // Same pathname must be same file.
  }

  /* Compute the chksumfile of each file to see if they are the same */
  if (!currentChksumComputed)
  {
    int err = chksumfile_bypathname(fs, pathname1, currentChksum);
    if (err < 0) {
      fprintf(stderr,"Can't checksum path %s\n", pathname1);
      return 0;
    }
    currentChksumComputed = 1;
  }

  if (chksumfile_compare(currentChksum, e->chksum) == 0) {
    numdiffchecksum++;
    return 0;  // Checksum mismatch, not the same file
  }
  /* Checksums match, do a content comparison */
  int fd1 = Fileops_open(pathname1);
  if (fd1 < 0) {
    fprintf(stderr, "Can't open path %s\n", pathname1);
    return 0;
  }

  int fd2 = Fileops_open(e->pathname);
  if (fd2 < 0) {
    Fileops_close(fd1);
    fprintf(stderr, "Can't open path %s\n", e->pathname);
    return 0;
  }

  int ch1, ch2;

  do {
    ch1 = Fileops_getchar(fd1);
    ch2 = Fileops_getchar(fd2);

    if (ch1 != ch2) {
      break; // Mismatch - exit loop with ch1 != ch2
    }
  } while (ch1 != -1);

  // if files match then ch1 == ch2 == -1

  Fileops_close(fd1);
  Fileops_close(fd2);

  if (ch1 == ch2) {
    numsamefiles++;
  } else {
    numdifferentfiles++;
  }

  return ch1 == ch2;
}

void
Pathstore_dumpstats(FILE *file)
{
  fprintf(file,
          "Pathstore:  %"PRIu64" stores, %"PRIu64" duplicates\n"
          "Pathstore2: %"PRIu64" compares, %"PRIu64" checksumdiff, "
          "%"PRIu64" comparesuccess, %"PRIu64" comparefail\n",
          numstores, numdups, numcompares, numdiffchecksum,
          numsamefiles, numdifferentfiles);
}
