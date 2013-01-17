#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simdisk.h"

/* file API implementation */

/* open an exisiting file for reading or writing */
int my_open (const char * path){
	int inum;
	int ret = -1;
	struct inode *this;     // remember to free
	int i, j, k;
	this = find(path);
	if (this == NULL)
		return -1;

	inum = this->num;
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (fd[i] == -1 || fd[i] == 0) {
			fd[i] = inum;
			ret = i;
			break;
		}
	}

	if (ret == -1) printf("No empty fd !\n");
	free(this);
	return ret;
}


/* open a new file for writing only */
int my_creat(const char * path) {
	int i = 0, j, k;
	int div;
	int newinode;
	int empty_dentry;
	char dir_name[MAX_FILE_NAME_LENGTH * MAX_LEVEL];
	char file_name[MAX_FILE_NAME_LENGTH];
	struct inode *cur;
	struct inode tmp_inode;
	struct dir tmp_direc;    

	while(path[i] != '\0') {
		if(path[i] == '/')
			div = i;
		i++;
	}
	strncpy(dir_name, path, div + 1);      // including the last '/'
	dir_name[div] = '\0';
	strncpy(file_name, path + div + 1, i - div - 1); 
	file_name[i - div - 1] = '\0';
	/*printf("Create file %s in the dir: %s\n", file_name, dir_name);*/

	cur = find(dir_name);
	if(cur == NULL) {
		printf("No such directory: %s\n", dir_name);
		return -1;
	}
	if(cur->type == FILE_TYPE) {
		printf("%s is a file\n", dir_name);
		return -1;
    	}
    
	/* Judged if exists a file with same name, otherwise find a empty position in the folder */
	for(i = 0; i < 10; i++) {
		if(cur->blocks[i] == -1) continue;
		read_block(cur->blocks[i], &tmp_direc);          // load dir block
		for(j = 0; j < 32; j ++){
			if(tmp_direc.dentry[j].inode != 0 && tmp_direc.dentry[j].type == FILE_TYPE && strcmp(tmp_direc.dentry[j].name, file_name) == 0) {
				printf("File already exists\n");
				return -1;
			}
		}
	}

	for(i = 0; i < 10; i ++) {
		if(cur->blocks[i] != -1) {
			read_block(cur->blocks[i], &tmp_direc);
			for(j = 0; j < 32; j ++){
				if(tmp_direc.dentry[j].inode == 0) {      // found empty dir entry, mark position
					empty_dentry = j;
					break;
				}
			}
			if(j < 32) break;
		}
		else {              // current dir block is full, get a new dir block
			cur->blocks[i] = get_block();
			read_block(cur->blocks[i], &tmp_direc);
			empty_dentry = 0;
			break;
		}
	}

	/* acquire inode for new file */
	newinode = get_inode();
	if(newinode == -1)
		return -1;
	tmp_direc.dentry[empty_dentry].inode = newinode;
	tmp_direc.dentry[empty_dentry].type = FILE_TYPE;
	if(strlen(file_name) < 32) tmp_direc.dentry[empty_dentry].length = 32;
	else tmp_direc.dentry[empty_dentry].length = 32;
	strncpy(tmp_direc.dentry[empty_dentry].name, file_name, strlen(file_name));
	write_block(cur->blocks[i], &tmp_direc);       // update dir block(added a new dirEntry)

	// set up new file's inode
	read_inode(newinode, &tmp_inode);
	tmp_inode.type = FILE_TYPE;
	tmp_inode.num = newinode;
	strcpy(tmp_inode.mode, "rw-rw-r--");       // default mode is 664
	tmp_inode.size = 0;              // in bytes
	// tmp_inode.uid = 0;
	// tmp_inode.gid = 0;
	strncpy(tmp_inode.name, file_name, strlen(file_name));
	tmp_inode.blocks[0] = get_block();
	for(i = 1; i < 10; i ++)
		tmp_inode.blocks[i] = -1;
	for(i = 0; i < 30; i ++)
		tmp_inode.ind_blocks[i] = -1;
	write_inode(newinode, &tmp_inode);

	/* open the file */
	int ret = -1;
	for (i = 0; i < MAX_OPEN_FILES; i++) {
		if (fd[i] == -1 || fd[i] == 0) {
			fd[i] = newinode;
			ret = i;
			break;
		}
	}
	if (ret == -1) printf("No empty fd !\n");

	free(cur);
	return ret;
}


/* sequentially read from a file */
int my_read(int fdd, void * buf, int cnt) {
	int i, j, k;
	int count = cnt;                               // Bytes remain to be read
	int blknum = (count - 1) / 1024 + 1;           // Allocated block num for data
	int indnum;
	// int fpoff = fp % 1024;
	struct inode this;
	struct inode *node = &this;
	struct inode *mi = (struct inode *)malloc(sizeof(struct inode));
	int len = 0;            // file pointer offset
	int temp = 0;           // every copy bytes
	int flag = 0;
	// int tempp = fp;
	unsigned char tmp[1024];

	read_inode(fd[fdd], &this);        // load file's inode

	/* only need direct blocks */
	if (blknum <= 10) {
		if (count > 1024) temp = 1024;             // remain Bytes greater than 1024(block space)?
		else temp = count;
		for (i = 0; i < blknum; i ++) {
			read_block(this.blocks[i], tmp);
			memcpy(buf + len, tmp, temp);          // copy 'temp' Bytes
			length += temp;                        // file offset 'temp' Bytes
		}
		break;
	}



	

	/* need indirect blocks, firstly read from direct blocks */
	for (i = 0; i < 10; i++) {
		temp = 1024;

		if (flag) {
			read_block(this.blocks[i], tmp);
			memcpy(buf + len - tempp, tmp, temp);
			fp += temp;
		} else {
			if (len + temp >= tempp) {
				temp = temp - fpoff;
				if (temp > 0) {
					read_block(this.blocks[i], tmp);
					memcpy(buf, tmp + fpoff, temp);
					fp += temp;
					len += fpoff;
				}
				flag = 1;
			}
		}
		len += temp;
	}

	blknum = blknum - 10;
	indnum = blknum / 256 + 1;  /* number of indirect blocks needed */

	/* read from indirect blocks */
	for (i = 0; i < indnum; i++) {
		struct ind_block ib;
		read_block(this.ind_blocks[i], (char *) &ib);

		for (j = 0; j < 256; j++) {
			temp = 1024;
			if ((temp + len) > count)
				temp = count - len;
			
			if (flag) {
		    		read_block(ib.blocks[j], tmp);
				memcpy(buf + len - tempp, tmp, temp);
				fp += temp;
			} else {
				if (len + temp >= tempp) {
					temp = temp - fpoff;
					if (temp > 0) {
						read_block(ib.blocks[j], tmp);
						memcpy(buf, tmp + fpoff, temp);
						fp += temp;
						len += fpoff;
					}
					flag = 1;
				}
			}
			len += temp;		
			if (blknum == (j + i * 256))
				break; 
		}
	}
	return len - tempp;
}