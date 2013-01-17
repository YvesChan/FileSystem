#ifndef SIMDISK_H
#define SIMDISK_H

#define MAX_FILE_NAME_LENGTH 50
#define MAX_FILE_NUM 10000   //amount of inode
#define MAX_OPEN_FILES 10
// #define BITMAP_START 1
#define BLOCK_MAP_START 1    // block_map cost 13 blocks, which is 13312 Bytes, and then 106496 bit, which means 106MB space
#define INODE_MAP_START 14   // inode_map cost 2 blocks, which is 16096 bits, actually use 10000 bit
// #define BITMAP_LENGTH 32
#define INODE_TABLE_START 16      //first inode block
#define BLOCK_TABLE_START 2516    //inode table costs 2500 blocks 
#define BLOCKSIZE 1024

#define FILE_TYPE 1
#define DIR_TYPE 2

#define MAX_LEVEL 10     //max level number of directory


int fd[MAX_OPEN_FILES];        // file description
char cwd_name[100];           // current working directory name
int cwd_inode;            // working directory's inode

// super block
struct super_block
{
	int magic_number;
	int block_num;          //total block number
	int inode_num;			//total inode number
	int free_blk;			//free block number
	int free_inode;			//free inode number
};

// each inode has 256 Bytes
struct inode {
	int type;                       /* 1 for file, 2 for dir */
	int num;                        /* inode number */
	int size;                        /* file size */
	int uid;						/* user id */
	int gid;						/* group id */
	char mode[11];					/* ACL of inode */
	char name[MAX_FILE_NAME_LENGTH];   /* file name */
	int blocks[10];                 /* direct blocks(10 KB) */
	int ind_blocks[30];            /* indirect blocks(7.5 MB) */
	//3-level indirect blocks
	char unused[15];
};

// indirect block's structure
struct ind_block
{
	int blocks[256];
};

//directory entry in block (32 bytes), each block has 32 dir entries at most
struct dirEntry
{
	int inode;          // dir entry inode
	int type;           // dir entry type: file or sub dir
	int length;         // file name's length: 32 or 64
	char name[20];      // file or sub dir name, can be extended to 50 Bytes
};


// directory block struct, has 32 dir entries space
struct dir
{
	struct dirEntry dentry[32];    // two 32B can cast to one 64B
};

//bitmap in block
struct bmap {
	unsigned char map[BLOCKSIZE];
};

// file API
extern int my_open (const char * path);
extern int my_creat (const char * path);
extern int my_read (int fd, void * buf, int count);
extern int my_write (int fd, const void * buf, int count);
extern int my_close (int fd);
extern int my_remove (const char * path);
extern int my_rename (const char * old, const char * new);

// directory API
extern int my_mkdir (const char * path);
extern int my_rmdir (const char * path);
extern int my_chdir (const char * path);

extern void my_mkfs (const char * path);
extern int my_info ();      //dumpe2fs

extern int my_cat ();
extern int my_cp ();


//provided by the lower layer. Device operation
typedef char block [BLOCKSIZE];

extern int dev_open (const char * path);
extern int read_block (int block_num, void * block);
extern int write_block (int block_num, void * block);
extern int read_inode (int inum, void * inode);
extern int write_inode (int inum, void * inode);
extern struct inode *find(const char *path);

//bitmap operation
extern void set_map(int blk, int i);
extern int get_inode();
extern int get_block();

#endif