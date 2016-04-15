#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include "disk_emu.h"
#include "sfs_api.h"

#define Inode_size 	100
#define block_size	1024
#define	block_num	2048
#define	max_file_size 
#define	Inode_block	6
#define	free_block	9
#define	dir_block	3	
#define	super_block	1

typedef struct SuperBlock{
	int magic;
	int blocksize;
	int fs_size;
	int inode_len;
	int root_dir;
}SuperBlock;

typedef struct Inode{
	int id;
	int size;
	int pointer[12];
	int single_indir;
}Inode; 

typedef struct Directory{
	char *name;
	char *extension;
	int inode;
}Directory;

typedef struct Single_indir{
	int address;
}Single_indir;

typedef struct FreeMap{
	int freeinode[Inode_size];
	int freeblock[block_num];
}FreeMap;

typedef struct Opentable{
	int Iid;
	int Isize;
}Opentable;

SuperBlock *superblock;
Inode *i_node;
Directory *root_dir;
Single_indir *sdir;
FreeMap *freemap;
Opentable *opentable;
int nextdir=1;

void mksfs(int fesh){

}

int searchfile(char *fname){

}

int sfs_get_next_filename(char *fname){

}

int sfs_GetFileSize(const char *path){

}

int sfs_fopen(char *name){

}

int sfs_fclose(int fileID){

}

int sfs_fread(int fileID, char *buf, int length){

}

int sfs_fwrite(int fileID, char *buf, int length){
	int id=-1;

	for(int i=0;i<Inode_size;i++){
		if((opentable+i)->Iid=fileID){
			id=fileID;break;}
	}
	if(id==-1)
		return -1;
	if((i_node+id)->size+length>max_file_size)
		return -1;

	int numb=((i_node+id)->size+length)/block_size;
	if(numb==1){
		(i_node+id)->size+=length;

		return (i_node+id)->size;
	}else if(numb<=9){


	}else{

	}

	return 1;
}

int sfs_fseek(int fileID, int loc){

}

int sfs_remove(char *file){

}