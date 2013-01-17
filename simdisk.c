#include "simdisk.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// int block_num = 102400       //total number of block in the whole disk


/* check to see if the device already has a file system on it,
 * and if not, create one. */
void my_mkfs (const char * path) {
	char *tmp = (char *) calloc(BLOCKSIZE, 1);        // allowcate in heap instead of stack and initial with 0
	read_block(0, tmp);             // super block
	if ( *(int *) tmp == 2012) {    // file system exists
		printf("File system has been created before.\n");
		free(tmp);
		return;
	}

	struct super_block *sb = (struct super_block *) calloc(BLOCKSIZE, 1);      // allowcate in heap
	struct inode *root = (struct inode *) malloc(sizeof(struct inode));
	struct dir *root_block = (struct dir *) malloc(sizeof(struct dir));
	int i;

	sb->magic_number = 2012;       // write the magic number
	sb->block_num = dev_open(path);
	sb->inode_num = MAX_FILE_NUM;
	sb->free_blk = 99883;    // total is 99884, minus 1 for root dir block
	sb->free_inode = 2499;   // total is 2500, minus 1 for root dir inode
	write_block(0, sb);      // write super block 
	// printf("Super block initialized\n");

	/* initialize fd */
	for(i = 0; i < MAX_OPEN_FILES; i++)
		fd[i] = -1;

	/* set bitmap in block 1 and 14: 1000 0000 */
	*tmp = 0x80;  
	write_block(BLOCK_MAP_START, tmp);
	*tmp = 0xc0;          // the first inode will be left empty, so the bitmap for inode should be 1100 0000
	write_block(INODE_MAP_START, tmp);

	/* root inode initial */
	root->type = DIR_TYPE;
	root->num = 1;                // the second inode is root inode, in block 16, inode number is 1, not 0
	root->size = 1;               // 1 KB for 1 block
	root->uid = 0;
	root->gid = 0;
	strcpy(root->mode, "rwxr-xr-x");
	strcpy(root->name, "/");
	root->blocks[0] = BLOCK_TABLE_START;     // root directory' block
	for(i = 1; i < 10; i++)
		root->blocks[i] = -1;    
	for(i = 0; i < 30; i++)
		root->ind_blocks[i] = -1;
	write_inode(1, root);
	/*printf("inode table initialized\n");*/

	/* root dir block initial */
	root_block->dentry[0].inode = 1;
	root_block->dentry[0].type = DIR_TYPE;
	root_block->dentry[0].length = 32;
	strcpy(root_block->dentry[0].name, ".");     // itself

	root_block->dentry[1].inode = 1;
	root_block->dentry[1].type = DIR_TYPE;
	root_block->dentry[1].length = 32;
	strcpy(root_block->dentry[1].name, "..");    // parents dir, also itself

	write_block(BLOCK_TABLE_START, root_block);

	free(tmp);
	free(sb);
	free(root);
	free(root_block);
	printf("File system initialized\n");
	// fp = 0;
}


/* find the inode of path, including dir or file, judged by the last char */
struct inode *find(const char *path) {
	struct inode *root = (struct inode *) malloc(sizeof(struct inode));
	struct inode *tmp = root;
	struct dir *direc = (struct dir *) malloc(sizeof(struct dir));
	int i = 0, j = 0, k = 0, absu = 0, d = 0;

	/* directory entry count */
	int count = 0;
	char arg[MAX_LEVEL][MAX_FILE_NAME_LENGTH];

	// if path is root dir, then return root inode
	if (!strcmp(path, "/")) {
		read_inode(1, root);
		return root;
	}

	/* parse the path , store each subdir in arg[] */
	if (path[0] == '/') {        // absulotely path
		i = 1;
		absu = 1;     
	}
	if (path[strlen(path) - 1] == '/')     // path is directory not file
		d = 1;
	while (path[i] != '\0') {
		if (j == MAX_LEVEL) {
			printf("path error\n");
			return NULL;
		}
		if (path[i] != '/') {
			arg[j][k] = path[i];
			i++;
			k++;
		} else {
			arg[j][k] = '\0';
			i++;
			j++;
			k=0;
		}
	}
	if (d == 1) count = j - 1;     // if dir
	else count = j;


	/* find the inode */
	if (absu == 1)     // if the path is absulutely
		read_inode(1, tmp);
	else 
		read_inode(cwd_inode, tmp);
	for (i = 0; i <= count; i++) {
		int flag = 0;
		for (j = 0; j < 10; j++) {
			if (tmp->blocks[j] <= 0) continue;
			read_block(tmp->blocks[j], direc);
			for (k = 0; k < 32; k ++) {
				if (direc->dentry[k].inode == 0) continue;
				if (d == 1 && direc->dentry[k].type == FILE_TYPE) continue;     //type matching
				if (d == 0 && direc->dentry[k].type == DIR_TYPE) continue;
				if (!strcmp(direc->dentry[k].name, arg[i])) {
					read_inode(direc->dentry[k].inode, tmp);
					flag = 1;            // found!
					break;
				}
				if(direc->dentry[k].length == 64) k++;      // cost two entry space
			}
			if (k < 32) break; 
		}
		if (!flag) {
			printf("%s not found!\n", path);
			return NULL;
		}
	}

	return tmp;
}
