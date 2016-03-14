// server
// has only one parameter which is server's buffer size

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

const char *share_region="/shm";		// shared memoty region

typedef struct Job
{
	int id;
	int sleeptime;
	int pages;
	int index;
}Job;

typedef struct J_buffer
{
	int start;
	int end;
	int size;
	int overload;
	sem_t full;					// lockers 
	sem_t empty;
	sem_t mutex;
	Job jobs[1];
}J_buffer;

J_buffer *shared_buffer;
Job job;

int key;						// gobal key variable to connect the memory segment

void setup_share_mem(int size){			// setup memory connection by the key
	key=shm_open(share_region,O_CREAT | O_RDWR,0666);
	if(key==-1){
		printf("share memory setup failed:%s\n", strerror(errno));
		exit(0);
	}
	ftruncate(key, sizeof(J_buffer)+size* sizeof(Job));
}

void attach_share_mem(){				// map with jobs buffer
	shared_buffer=(J_buffer *) mmap(
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
void init_semaphore(int size){			// initialize the semaphore lockers' values and the job buffer values
	sem_init(&shared_buffer->full,1,0);
	sem_init(&shared_buffer->empty,1,size);
	sem_init(&shared_buffer->mutex,1,1);
	shared_buffer->start=0;
	shared_buffer->end=0;
	shared_buffer->size=size;
	shared_buffer->overload=0;
}

void take_a_job(){						// read the jobs from shared memory
	sem_wait(&shared_buffer->full);
	sem_wait(&shared_buffer->mutex);
	job=shared_buffer->jobs[shared_buffer->start];
	shared_buffer->start=(shared_buffer->start+1) % shared_buffer->size;
	printf("\nClient %d has %d pages to print, puts into buffer[%d]\n",job.id,job.pages,job.index);
}
void print_a_msg(){						// print out message
	printf("Printer starts printing %d pages from buffer[%d]\n", job.pages, job.index);
	sem_post(&shared_buffer->mutex);
	sem_post(&shared_buffer->empty);
}
void go_sleep(){						// sleep the process while printing
	int sleeping=job.sleeptime;
	sleep(sleeping);
	printf("Printer finishes printing %d pages from buffer[%d]\n", job.pages, job.index);
}

int main(int argc,char *argv[]){
	if(argc<2){							// check the number of parameter inputs
		printf("Not enough parameters.\n");return -1;}
	int size=atoi(argv[1]);
	if(size<=0){						// server buffer size cannot less than zero
		printf("Buffer size should be positive number.\n");return -1;}
	setup_share_mem(size);
	attach_share_mem();
	init_semaphore(size);
	printf("\n*********\n");
	while(1){
		sem_getvalue(&shared_buffer->full,&shared_buffer->overload);
		if(shared_buffer->overload==0)
			printf("\nNo request in buffer, printer sleeps\n");
		take_a_job();
		print_a_msg();
		if(shared_buffer->overload>=size-1)
			printf("Buffer is full, printer sleeps\n");
		go_sleep();
	}
	return 0;
}
