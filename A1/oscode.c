#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

int cmdcounter=0;
struct historycmd
{
	char *args[20];
	int err;
    int num;
    int bg;
}historycmd[10];			// history array

struct pidjobs
{
    int pid;
    char *name;
}pidjobs[100];				// jobs array

int getcmd(char *prompt, char *args[], int *background)
{
    int length, i = 0;
    char *token, *loc;
    char *line;
    size_t linecap = 0;

    printf("%s", prompt);
    length = getline(&line, &linecap, stdin);

    if (length <= 0) {
        exit(-1);
    }
    
    // Check if background is specified..
    if ((loc = index(line, '&')) != NULL){
        *background = 1;
        *loc = ' ';
    } else
        *background = 0;

    while ((token = strsep(&line, " \t\n")) != NULL){
        for (int j = 0; j < strlen(token); j++)
            if (token[j] <= 32)
                token[j] = '\0';
        if (strlen(token) > 0)
            args[i++] = token;
    }
    args[i++]=NULL;
    return i;
}

int redirection(char *args[]){				// function to check if input contains redirection symbol
	for(int i=0; args[i]!=NULL;i++){
		if(strcmp(">",args[i])==0){
			return i;}
	}
	return 0;
}

void history_add(char *args[],int error,int num,int bg){		// method to add command into history list
    int p=(num-1)%10;
    for(int i=0;args[i]!=NULL;i++){
        historycmd[p].args[i]=args[i];
    }
    historycmd[p].err=error;
    historycmd[p].num=num;
    historycmd[p].bg=bg;
}  

void commandexec(char *args[],int bg){			// function to execute all the command
    int cd=strcmp("cd",args[0]);
    int pwd=strcmp("pwd",args[0]);
    int fg=strcmp("fg",args[0]);
    int jobs=strcmp("jobs",args[0]);
    int exits=strcmp("exit",args[0]);
    int his=strcmp("history",args[0]);
    int err=0;					// error flag for history
    if(cd==0){
    	chdir(args[1]);
    }
    else if(pwd==0){
        char *buf=malloc(sizeof(char)*1000);
        getcwd(buf,100);
        printf("%s\n",buf);
        free(buf);
    }
    else if(fg==0){
    	if(args[1]==NULL){
    		printf("error: an index needed\n");
    		err=1;
    	}
    	else{
    		int check=atoi(args[1]);
    		if(check==0){
    			printf("error: index should be integer\n");
    			err=1;
    		}
    		else{
    			int fground=waitpid(pidjobs[check-1].pid,NULL,WCONTINUED);
    			if(fground==-1){
    				printf("fg fail\n");
    			}else
    				printf("fg success\n");
    		}
    	}
    }
    else if(jobs==0){				//list jobs in background
     	for(int i=0;i<100;i++){
     		if(pidjobs[i].pid!=0){
     			if(waitpid(pidjobs[i].pid,NULL,WNOHANG)==-1){
     				pidjobs[i].pid=0;
     			}else
     				printf("[%d]-- pid:%d  command:%s\n",i+1,pidjobs[i].pid,pidjobs[i].name);
     		}
     	}
    }
    else if(his==0){				// list the history command
    	for(int i=0;historycmd[i].args[0]!=NULL;i++){
            printf("history %d:  %s,  error(%d)\n", historycmd[i].num,historycmd[i].args[0],historycmd[i].err);
        }
    }
    else if(exits==0){
        exit(0);
    }
    else{
    	int redir=redirection(args);
        int id=fork();
        if(id==0){             //child process
        	if(redir!=0){      // redirection when detects ">"
        		args[redir]=NULL;
            	FILE *fp=freopen(args[redir+1],"w+",stdout);
            	if(execvp(args[0],args)==-1){
            		printf("bash: %s: command not found\n", args[0]);
                    err=1;
            		// exit(0);
            	}
           		fclose(fp);
        	}
            else if(execvp(args[0],args)==-1){
                printf("bash: %s: command not found\n", args[0]);
                err=1;			// if command is illegal, set error flag to be 1
            	// exit(0);
            }
        }else {                 //parent process
            if(bg){             //add process into jobs array when bg enable
                for(int i=0;i<100;i++){
                    if(pidjobs[i].pid==0){
                        pidjobs[i].pid=id;
                        pidjobs[i].name=strdup(args[0]);
                        break;
                    }
                }
            }else{				// background not enable
				pid_t child=waitpid(id,NULL,WCONTINUED);
            }
        }
    }
    cmdcounter++;
    history_add(args,err,cmdcounter,bg);		// add executed cmommand into historty array
}

int main()
{
    char *args[20];
    int bg;
    
	while(1){
        
        int cnt = getcmd("\n>>  ", args, &bg);
        
        for (int i = 0; i < cnt; i++)
            printf("\nArg[%d] = %s", i, args[i]);
        if (bg)
            printf("\nBackground enabled..\n");
        else
            printf("\nBackground not enabled \n");
        
        if(args[0]==NULL){
    		// do nothing if enter nothing
    	}else{
    		int history=atoi(args[0]);			// check if input is number and execute history things
	        if (history>0){
                int p=(history-1)%10;
	            if(historycmd[p].num!=history)
	                printf("No commamd found in history\n");
	            else{
                    if(historycmd[p].err==0){			// only execute when history command is not an error
                        commandexec(historycmd[p].args,historycmd[p].bg);
                    }else
                        printf("history commamd error\n");
	            }
	        }else {
	            commandexec(args,bg);
	        }	
    	}
    }
}
