// client
// first parameter is client id, second one is sleep time,
// and third one is number of pages.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <time.h>
#include <semaphore.h>

const char *share_region="/shm";		// shared memory region

typedef struct Job
{
	int id;
	int sleeptime;
	int pages;
	int index;				// index = buffer position
}Job;

typedef struct J_buffer
{
	int start;
	int end;
	int size;
	int overload;
	sem_t full;				// semaphore lockers
	sem_t empty;
	sem_t mutex;
	Job jobs[1];
}J_buffer;

J_buffer *shared_buffer;
Job job;

int key;					// gobal key variable to connect the memory segment

void attach_share_mem(){	// use the key to setup the memoty connection
	key=shm_open(share_region,O_RDWR,0666);
	if(key==-1){
		printf("Failed: server is not running.\n");
		exit(1);
	}
	ftruncate(key, sizeof(J_buffer));
}

void place_parameters(){	// map into jobs buffer
	shared_buffer= (J_buffer *) mmap(
		0,
		sizeof(J_buffer), 
		PROT_READ | PROT_WRITE, 
		MAP_SHARED, 
		key,
		0);
	if(shared_buffer==MAP_FAILED){
		printf("Map Failed:%s\n", strerror(errno));
		exit(0);
	}
}

void get_job(long id,long sleeptime,long pages){		// place all parameters into job struct
	job.id=(int)id;
	job.sleeptime=(int)sleeptime;
	job.pages=(int)pages;
	job.index=(int)shared_buffer->end;
}

void put_job(){				// put the job into shared memory 
	sem_getvalue(&shared_buffer->full,&shared_buffer->overload);
	if(shared_buffer->overload>=shared_buffer->size-1)
		printf("\nWait for server, buffer is full\n");
	sem_wait(&shared_buffer->empty);
	sem_wait(&shared_buffer->mutex);
	shared_buffer->jobs[shared_buffer->end] = job;
	shared_buffer->end = (shared_buffer->end+1) % shared_buffer->size;
	sem_post(&shared_buffer->mutex);
	sem_post(&shared_buffer->full);
	printf("\nClient %d has %d pages to print, puts into buffer[%d]\n",job.id,job.pages,job.index);
}
void release_share_mem(){	// release the memory region
	shmdt(shared_buffer);
}

int main(int argc,char *argv[]){
	if(argc<4){				// check the number of parameter from inputs
		printf("Not enough parameters.\n");return -1;}
	attach_share_mem();
	place_parameters();
	if(shared_buffer->size<=0){		// check if server has space to recive message
		printf("Failed: server is not running.\n");return -1;}
	long id=strtol(argv[1],NULL,10);
	long sleep=strtol(argv[2],NULL,10);
	long pages=strtol(argv[3],NULL,10);
	get_job(id,sleep,pages);
	put_job();
	release_share_mem();
	return 0;
}