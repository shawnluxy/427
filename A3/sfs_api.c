#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>
#include "disk_emu.h"
#include "sfs_api.h"

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

typedef struct Directory_table{
	char *name;
	char *extension;
	int inode;
}Directory_table;

typedef struct Single_indir{
	int addr[block_size/4];
}Single_indir;

typedef struct FreeMap{
	int freeinode[Inode_size];
	int freeblock[block_num];
}FreeMap;

typedef struct Opentable{
	int Iid;
	int Iset;
}Opentable;

SuperBlock *superblock;
Inode *i_node;
Directory_table *root_dir;
Single_indir *sdir;
FreeMap *freemap;
Opentable *opentable;
int nextdir=0;

void createfile(int inode,int size, int dir, char *name, char *extension);
void writetodisk(){
	write_blocks(super_start_block,1,superblock);
	write_blocks(inode_start_block,6,i_node);
	write_blocks(dir_start_block,3,root_dir);
	write_blocks(free_start_block,9,freemap);
}
void readfromdisk(){
	read_blocks(super_start_block,1,superblock);
	read_blocks(inode_start_block,6,i_node);
	read_blocks(dir_start_block,3,root_dir);
	read_blocks(free_start_block,9,freemap);
}

void mksfs(int fesh){
	superblock=calloc(1,sizeof(SuperBlock));
	i_node=calloc(Inode_size,sizeof(Inode));
	root_dir=calloc(Inode_size+3,sizeof(Directory_table));
	opentable=calloc(Inode_size,sizeof(Opentable));
	freemap=calloc(1+1,sizeof(FreeMap));
	nextdir=0;
	for(int i=0;i<Inode_size;i++){
		freemap->freeinode[i]=0;
	}
	for(int i=0;i<block_num;i++){
		if(i<=19)
			freemap->freeblock[i]=1;
		else
			freemap->freeblock[i]=0;
	}
	if(fesh){
		superblock->magic=0xACBD0005;
		superblock->blocksize=block_size;
		superblock->fs_size=block_num;
		superblock->inode_len=Inode_size;
		superblock->root_dir=0;
		char rootname[]="root";
		createfile(0,0,0,rootname,NULL);
		i_node->pointer[0]=dir_start_block+0;
		i_node->pointer[1]=dir_start_block+1;
		i_node->pointer[2]=dir_start_block+2;
		init_fresh_disk("sfs",block_size,block_num);
		writetodisk();
	}else{
		init_disk("sfs",block_size,block_num);
		readfromdisk();
	}
}
int searchfreeblock(){
	for(int i=20;i<block_num;i++){
		if(freemap->freeblock[i]==0)
			return i;
	}
	return -1;
}

int searchfile(char *fname, char *name, char *extension){
	if(strlen(fname)>20)
		return -1;
	char *p;
	char s[1]=".";
	p=strchr(fname,'.');
	int dot=(int)(p-fname);
	if(dot<0 || dot>=strlen(fname)-1){
		strcpy(name,fname);
		extension=NULL;
	}else if(dot==0){
		printf("Invalid File Name.\n");
	}else{
		char temp[20];
		strcpy(temp,fname);
		strcpy(name,strtok(temp,s));
		strcpy(extension,strtok(NULL,s));
	}

	for(int i=0;i<Inode_size;i++){
		if((root_dir+i)->name!=NULL){
			if(strcmp(name,(root_dir+i)->name)==0){
				if(extension==NULL){
					return (root_dir+i)->inode;
				}else{
					if(strcmp(extension,(root_dir+i)->extension)==0)
						return (root_dir+i)->inode;
					else
						return -1;
				}
			}
		}
	}
	return -1;
}
void createfile(int inode,int size, int dir, char *name, char *extension){
	(i_node+inode)->id=inode;
	(i_node+inode)->size=size;
	(root_dir+dir)->name=(char *)malloc(name_limit);
	(root_dir+dir)->extension=(char *)malloc(extension_limit);
	strcpy((root_dir+dir)->name,name);
	if(extension==NULL)
		(root_dir+dir)->extension=NULL;
	else
		strcpy((root_dir+dir)->extension,extension);
	(root_dir+dir)->inode=inode;
	freemap->freeinode[inode]=1;
}

int searchopentable(int fileID){
	for(int i=0;i<Inode_size;i++){
		if((opentable+i)->Iid==fileID){
			return i;
		}
	}
	return -1;
}

int sfs_get_next_filename(char *fname){
	if((root_dir+nextdir)->name==NULL)
		return 0;
	strcpy(fname,(root_dir+nextdir)->name);
	if((root_dir+nextdir)->extension!=NULL){
		strcat(fname,".");
		strcat(fname,(root_dir+nextdir)->extension);
	}
	nextdir++;
	// printf("%s\n", fname);
	return 1;
}

int sfs_GetFileSize(const char *path){
	char *name=(char *)malloc(20); 
	char *extension=(char *)malloc(20);
	char *fname=(char*)path;
	if(fname==NULL)
		return -1;
	int inode=searchfile(fname,name,extension);
	if(inode==-1)
		return -1;
	else
		return (i_node+inode)->size;
}

int sfs_fopen(char *fname){
	char *name=(char *)malloc(20); 
	char *extension=(char *)malloc(20);
	if(fname==NULL)
		return -1;
	if(strlen(fname)>20)
		return -1;
	int inode=searchfile(fname,name,extension);
	int dir;
	//check file name limit
	if(strlen(name)>16){
		printf("exceed file name limit\n");
		return -1;
	}else if(strlen(extension)>3){
		printf("exceed file extension limit\n");
		return -1;
	}
	if(inode==-1){
		//find free inode
		for(int i=0;i<Inode_size;i++){
			if(freemap->freeinode[i]==0){
				inode=i;
				break;
			}
		}
		if(inode==-1){
			printf("no space\n");
			return -1;
		}
		//find free directory
		for(int i=0;i<Inode_size;i++){
			if((root_dir+i)->inode==0){
				dir=i;
				break;
			}
		}
		createfile(inode,0,dir,name,extension);
		// printf("???%s.%s???\n", name,extension);
		for(int i=0;i<Inode_size;i++){
			if((opentable+i)->Iid==0){
				(opentable+i)->Iid=inode;
				(opentable+i)->Iset=0;
				break;
			}
		}
	}else{
		for(int i=0;i<Inode_size;i++){
			if((opentable+i)->Iid==inode)
				return inode;
		}
		for(int i=0;i<Inode_size;i++){
			if((opentable+i)->Iid==0){
				(opentable+i)->Iid=inode;
				(opentable+i)->Iset=(i_node+inode)->size;
				break;
			}
		}
	}
	writetodisk();
	return inode;
}

int sfs_fclose(int fileID){
	int id=searchopentable(fileID);
	if(id==-1)
		return -1;
	(opentable+id)->Iid=0;
	(opentable+id)->Iset=0;
	return 0;
}

int sfs_fseek(int fileID, int loc){
	int id=searchopentable(fileID);
	if(id==-1)
		return -1;
	(opentable+id)->Iset=loc;
	return 0;
}

int sfs_fread(int fileID, char *buf, int length){
	int id=searchopentable(fileID);
	if(id==-1)
		return -1;

	int startblock=((opentable+id)->Iset)/block_size;
	int startpoint=((opentable+id)->Iset)%block_size;
	int inode=(opentable+id)->Iid;
	if((startpoint+length)>(i_node+inode)->size)
		length=(i_node+inode)->size-(opentable+id)->Iset;
	int numblock=1+(startpoint+length)/block_size;
	char *reader=calloc(numblock,block_size);
	if(numblock<=12){
		//read i_node pointer from disk
		for(int i=startblock;i<numblock;i++){
			char *temp=calloc(block_size,1);
			if((i_node+inode)->pointer[i]==0)
				return -1;
			read_blocks((i_node+inode)->pointer[i],1,temp);
			memcpy(reader+block_size*(i-startblock),temp,block_size);
			free(temp);
		}
	}else{
		sdir=calloc(1,sizeof(Single_indir));
		//read frome disk
		for(int i=startblock;i<12;i++){
			char *temp=calloc(block_size,1);
			if((i_node+inode)->pointer[i]==0)
				return -1;
			read_blocks((i_node+inode)->pointer[i],1,temp);
			memcpy(reader+block_size*(i-startblock),temp,block_size);
			free(temp);
		}
		//load single indirect
		read_blocks((i_node+inode)->single_indir,1,sdir);
		for(int i=0;i<(numblock-12);i++){
			char *temp=calloc(block_size,1);
			if(sdir->addr[i]==0)
				return -1;
			read_blocks(sdir->addr[i],1,temp);
			memcpy(reader+block_size*(i+12-startblock),temp,block_size);
			free(temp);
		}
	}
	for(int i=0;i<length;i++){
		*(buf+i)=*(reader+startpoint+i);
	}
	// memcpy(buf,reader+startpoint,length);
	sfs_fseek(fileID,(opentable+id)->Iset+length);
	free(reader);
	return length;
}

int sfs_fwrite(int fileID, char *buf, int length){
	int id=searchopentable(fileID);
	if(id==-1)
		return -1;
	if((opentable+id)->Iset+length>max_file_size)
		return -1;

	int numblock, startpoint,inode;
	startpoint=(opentable+id)->Iset;
	inode=(opentable+id)->Iid;
	if((startpoint+length)>(i_node+inode)->size){
		(i_node+inode)->size=(startpoint+length);
	}
	numblock=1+((i_node+inode)->size)/block_size;
	// printf("++++%s\n", buf);
	// printf("%d//%d\n", strlen(buf),length);
	char *writer=calloc(numblock,block_size);
	if(numblock<=12){
		//copy everything into writer buffer
		for(int i=0;i<numblock;i++){
			int bp=(i_node+inode)->pointer[i];
			if(bp!=0){
				char *temp=calloc(block_size,1);
				read_blocks(bp,1,temp);
				memcpy(writer+block_size*i,temp,block_size);
				freemap->freeblock[bp]=0;
				free(temp);
			}else{
				char *empty=calloc(block_size,1);
				memcpy(writer+block_size*i,empty,block_size);
				free(empty);
			}
		}
		// change or add buf into writer
		if(length>strlen(buf)){
			for(int i=0;i<strlen(buf);i++){
				*(writer+startpoint+i)=*(buf+i);
			}
			// char s[1]=".";
			// for(int j=strlen(buf);j<length;j++){
			// 	*(writer+j)=*(s);	
			// }
		}else{
			for(int i=0;i<length;i++){
				*(writer+startpoint+i)=*(buf+i);
			}
		}
		// memcpy(writer+startpoint,buf,length);
		// printf("????%s\n", writer);
		//writer back to i_node pointer and disks
		for(int i=0;i<numblock;i++){
			int freebp=searchfreeblock();
			(i_node+inode)->pointer[i]=freebp;
			write_blocks(freebp,1,writer+block_size*i);
			freemap->freeblock[freebp]=1;
		}
	}else{
		sdir=calloc(1,sizeof(Single_indir));
		if((i_node+inode)->single_indir<=19){
			(i_node+inode)->single_indir=searchfreeblock();
			freemap->freeblock[(i_node+inode)->single_indir]=1;
		}else{
			read_blocks((i_node+inode)->single_indir,1,sdir);
		}
		//copy 12 pointer into writer buffer
		for(int i=0;i<12;i++){
			int bp=(i_node+inode)->pointer[i];
			if(bp!=0){
				char *temp=calloc(block_size,1);
				read_blocks(bp,1,temp);
				memcpy(writer+block_size*i,temp,block_size);
				freemap->freeblock[bp]=0;
				free(temp);
			}else{
				char *empty=calloc(block_size,1);
				memcpy(writer+block_size*i,empty,block_size);
				free(empty);
			}
		}
		//copy single indirect into writer duffer
		for(int i=0;i<(numblock-12);i++){
			int sp=sdir->addr[i];
			if(sp!=0){
				char *temp=calloc(block_size,1);
				read_blocks(sp,1,temp);
				memcpy(writer+block_size*(i+12),temp,block_size);
				freemap->freeblock[sp]=0;
				free(temp);
			}else{
				char *empty=calloc(block_size,1);
				memcpy(writer+block_size*(i+12),empty,block_size);
				free(empty);
			}
		}
		//change or add buf into writer
		if(length>strlen(buf)){
			for(int i=0;i<strlen(buf);i++){
				*(writer+startpoint+i)=*(buf+i);
			}
			// char s[1]=".";
			// for(int j=strlen(buf);j<length;j++){
			// 	*(writer+j)=*(s);	
			// }
		}else{
			for(int i=0;i<length;i++){
				*(writer+startpoint+i)=*(buf+i);
			}
		}
		// memcpy(writer+startpoint,buf,length);
		//writer back to i_node pointer and disks
		for(int i=0;i<12;i++){
			int freebp=searchfreeblock();
			(i_node+inode)->pointer[i]=freebp;
			write_blocks(freebp,1,writer+block_size*i);
			freemap->freeblock[freebp]=1;
		}
		for(int i=0;i<(numblock-12);i++){
			int freesp=searchfreeblock();
			sdir->addr[i]=freesp;
			write_blocks(freesp,1,writer+block_size*(i+12));
			freemap->freeblock[freesp]=1;
		}
		write_blocks((i_node+inode)->single_indir,1,sdir);
	}
	sfs_fseek(fileID,startpoint+length);
	writetodisk();
	free(writer);
	return length;
}

int sfs_remove(char *file){
	char *name=(char *)malloc(20); 
	char *extension=(char *)malloc(20);
	int inode=searchfile(file,name,extension);
	if(inode==-1)
		return -1;
	//remove directory
	for(int i=0;i<Inode_size;i++){
		if((root_dir+i)->inode==inode){
			(root_dir+i)->name=NULL;
			(root_dir+i)->extension=NULL;
			(root_dir+i)->inode=0;
			break;
		}
	}
	//remove opentable(not necessary)
	for(int i=0;i<Inode_size;i++){
		if((opentable+i)->Iid==inode){
			(opentable+i)->Iid=0;
			(opentable+i)->Iset=0;
			break;
		}
	}
	//remove i_node and single indirect
	int numblock=(i_node+inode)->size/block_size;
	for(int i=0;i<12;i++){
		if((i_node+inode)->pointer[i]>19){
			char *temp=malloc(block_size);
			write_blocks((i_node+inode)->pointer[i],1,temp);
			freemap->freeblock[(i_node+inode)->pointer[i]]=0;
			(i_node+inode)->pointer[i]=0;
			free(temp);
		}
	}
	if((i_node+inode)->single_indir>19){
		if(numblock<12)
			return -1;
		sdir=calloc(1,sizeof(Single_indir));
		read_blocks((i_node+inode)->single_indir,1,sdir);
		for(int i=0;i<=(numblock-12);i++){
			if(sdir->addr[i]>19){
				char *temp=malloc(block_size);
				write_blocks(sdir->addr[i],1,temp);
				freemap->freeblock[sdir->addr[i]]=0;
				sdir->addr[i]=0;
				free(temp);
			}
		}
		(i_node+inode)->single_indir=0;
	}
	(i_node+inode)->id=0;
	(i_node+inode)->size=0;
	freemap->freeinode[inode]=0;
	writetodisk();
	return 0;
}