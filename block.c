#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdio.h>
#include "simdisk.h"

/* only open the file once */
static int f = -1;
static int devsize = 0;

/* returns the device size (in blocks) if the operation is successful,
 * and -1 otherwise */
int dev_open (const char * path)
{
  struct stat st;

  if (f < 0) {
    f = open (path, O_RDWR);
    if (f < 0) {
      perror ("open");
      return -1;
    }
    if (fstat (f, &st) < 0) {
      perror ("fstat");
      return -1;
    }
    devsize = st.st_size / BLOCKSIZE;
  }
  return devsize;
}

/* returns 0 if the operation is successful, and -1 otherwise 
 * read the block_num's information into *block */
int read_block (int block_num, void * block)
{
  if (block_num >= devsize) {
    printf ("block number requested %d, maximum %d", block_num, devsize - 1);
    return -1;
  }
  if (lseek (f, block_num * BLOCKSIZE, SEEK_SET) < 0) {
    perror ("lseek");
    return -1;
  }
  if (read (f, block, BLOCKSIZE) != BLOCKSIZE) {
    perror ("read");
    return -1;
  }
  return 0;
}

/* returns 0 if the operation is successful, and -1 otherwise */
int write_block (int block_num, void * block)
{
  if (block_num >= devsize) {
    printf ("block number requested %d, maximum %d\n", block_num, devsize - 1);
    return -1;
  }
  if (lseek (f, block_num * BLOCKSIZE, SEEK_SET) < 0) {
    perror ("lseek");
    return -1;
  }
  if (write (f, block, BLOCKSIZE) != BLOCKSIZE) {
    perror ("write");
    return -1;
  }
  if (fsync (f) < 0)
    perror ("fsync");		/* but return success anyway */
  return 0;
}

int read_inode (int inum, void * inode)
{
    if(inum >= MAX_FILE_NUM || inum == 0) {
        printf("inode number requested %d, maximum %d\n, minimum 1", inum, MAX_FILE_NUM - 1);
        return -1;
    }
    if (lseek(f, INODE_TABLE_START * BLOCKSIZE + inum * 256, SEEK_SET) < 0)
    {
        perror("lseek");
        return -1;
    }
    if (read(f, inode, 256) != 256)
    {
        perror("read");
        return -1;
    }

    return 0;
}

int write_inode (int inum, void * inode)
{
    if (inum >= devsize || inum == 0) {
        printf ("inode number requested %d, maximum %d\n", inum, MAX_FILE_NUM - 1);
        return -1;
    }
    if (lseek(f, INODE_TABLE_START * BLOCKSIZE + inum * 256, SEEK_SET) < 0)
    {
        perror("lseek");
        return -1;
    }
    if (write (f, inode, 256) != 256) 
    {
        perror ("write");
        return -1;
    }
  if (fsync (f) < 0)
    perror ("fsync");       /* but return success anyway */
  return 0;
}