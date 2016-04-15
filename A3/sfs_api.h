void mksfs(int fresh);
int sfs_get_next_filename(char *fname);
int sfs_GetFileSize(const char* path);
int sfs_fopen(char *name);
int sfs_fclose(int fileID);
int sfs_fwrite(int fileID, char *buf, int length);
int sfs_fread(int fileID, char *buf, int length);
int sfs_fseek(int fileID, int loc);
int sfs_remove(char *file);

#define Inode_size	100
#define	block_num	2048	
#define block_size 	1024
#define	name_limit	16
#define	extension_limit	3
#define	inode_start_block	1//6
#define	free_start_block	10//9
#define	dir_start_block		7//3
#define	super_start_block	0//1
#define max_file_size	274432
