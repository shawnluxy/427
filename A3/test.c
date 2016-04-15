#include <stdio.h>
#include <stdlib.h> 
#include <string.h>
#include <unistd.h>

typedef struct FreeMap{
	int freeinode[100];
	int freeblock[2048];
}FreeMap;

void main(){
	char str[]="test----.xml";
	char s[3]=".";
	char *t, *p;
	char *cc=str;
	char *ss=s;
	p=strpbrk(cc,ss);
	if (p==NULL)
		printf("no extension\n");
	
	t=strtok(cc,ss);
	if (t==NULL)
		printf("error\n");
	else
		printf("%s\n",t);


	t=strtok(NULL,ss);
	if (t==NULL)
		printf("error\n");
	else
		printf("%s\n",t);

	printf("%d\n", (int)sizeof(FreeMap));
	printf("%d\n", strcmp(cc,"lll"));
}