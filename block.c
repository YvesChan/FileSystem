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
            error ("open");
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

/* acquire free block, return block num */
int get_block() {
    int found = 0;
    int i, j, k;
    int retval;
    unsigned char bitb;
    char bits[BLOCKSIZE];
    for (i = BLOCK_MAP_START; i < INODE_MAP_START; i++) {     // i locate block num
        read_block(i, bits);
        for (j = 0; j < BLOCKSIZE; j++) {                     // j locate Byte num
            if (bits[j] != 0xffffffff) {   /* found! Why not 0xff ?*/
                found = 1;
                break;
            }
        }
        if (found) break;
    }
    bitb = (unsigned char) bits[j];

    for(k = 0; k < 8; k++)           // k locate bit
        if((~(bitb << k) & 0x80) != 0) break;
    retval = k + j * 8 + (i - BLOCK_MAP_START) * 8192 + BLOCK_TABLE_START;       // in bit units
    if(retval >= 102400) {
        printf("Error : No free block is founded");
        return -1;
    }
    bits[retval >> 3] |= (1 << (8 - 1 - (retval & 0x07)));             // set block bitmap
    write_block(i, bits);

    return retval;
}

/* acquire free inode, return inode num */
int get_inode() {
    int found = 0;
    int i, j, k;
    int ret;
    unsigned char bitb;
    char bits[BLOCKSIZE];
    for(i = INODE_MAP_START; i < INODE_TABLE_START; i ++) {
        read_block(i, bits);
        for(j = 0; j < BLOCKSIZE; j ++){
            if(bits[j] != 0xff) {
                found = 1;
                break;
            }
        }
        if(found) break;
    }

    bitb = (unsigned char)bits[j];
    for(k = 0; k < 8; k ++)
        if((~(bitb << k) & 0x80) != 0) break;

    ret = k + j * 8 + (i - INODE_MAP_START) * 8192 + INODE_TABLE_START;
    if (ret >= MAX_FILE_NUM) {
         printf("Error : No free inode is founded");
         return -1;
    }
    bits[ret >> 3] |= (1 << (8 - 1 - (ret & 0x07)));             // set inode bitmap
    write_block(i, bits);

    return ret;
}