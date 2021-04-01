#include <stdio.h>
#include <mqueue.h>
#include <sys/types.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <stdbool.h>

#define COLOR "\033[1;37;100m"

int input(char** inp){

	char str[1024];
	printf("arigato$>> ");
	fgets(str, 1024 ,stdin);
        	
        str[strlen(str)-1] = '\0'; //avoid garbage value
        *inp = str;
	return 1;
        
}


int checkPipe(char* str, char** pipe){
	int i = 0;
	char* delim = strtok(str, "|");
	while(delim != NULL){   // divide string over | characters
		pipe[i] = delim;
		i++;
		delim = strtok(NULL,"|");
	}
	if(pipe[1] == NULL){
		return 0;
	}else{
		
		pipe[i] = NULL;
		return 1;
	}
	
}

void pipelessExecution(char** cmnds, int i){

	pid_t child0;
	child0 = fork();
	
	if(child0 < 0)
		printf("Child process couldn't be created\n");
	else if(child0 == 0){
		if(execvp(cmnds[0], cmnds)<0)
			printf("Command is not executed\n");
		exit(0);
		
	}else{// make the parent wait
		int status;
		waitpid(child0, &status, 0);
	}
}


void pipedExecution(char** cmndLeft, char** cmndRight, int i, int i2){

	int pipefd[2];
	if (pipe(pipefd)) {
        	perror("pipe");
        	exit(0);
    	}
    	pid_t child0 = 0, child1 = 0;
    	child0 = fork();
    	if(child0 != 0)
    		child1 = fork();
	if(child0< 0){
        	printf("Fork failed\n");
        	exit(0);
	}else if(child0 == 0){
        	close(pipefd[0]);
        	dup2(pipefd[1], 1); //pipe management
        	close(pipefd[1]);
       	execvp(cmndLeft[0], cmndLeft);
       	printf("Cannot execute\n");
       	exit(0);
        }else{//Parent
        	close(pipefd[1]); // close for parent
        	if(child1 == 0){
        		close(pipefd[1]);  
        		dup2(pipefd[0], 0); //pipe management
        		close(pipefd[0]);  

       		execvp(cmndRight[0], cmndRight);
       		printf("Exec failed\n");
       		exit(0);
        	}else if(child1 != 0){
        		close(pipefd[0]);//make the parent wait for children
        		int status;
        		waitpid(child0,&status,0);
        		waitpid(child1,&status, 0);
        	}
        }
}
	

void divideSpaces(char* str, char** cmnds, int* noArgs){
	
	char* delim;
	int i = 0;

	delim = strtok(str, " "); // divide over spaces
	while( delim != NULL ) {
		if(strcmp(delim, " " ) != 0){ // to enable user to write commands like " ls     -l" and still work 
				if(strcmp(delim, "producer") == 0){
					delim = "./producer";
				}else if(strcmp(delim, "consumer") == 0)
					delim = "./consumer";
				*(cmnds+ i) = delim;
		}
		if(strlen(cmnds[i]) != 0){// adjacent spaces will be ignored
			i++;
		}
		delim = strtok(NULL, " ");
	
	}
	*(cmnds + i) = NULL; // last arg to NULL to run exec and avoid garbage value
	*noArgs = i;
	
}


void tappedMode(char** cmndLeft, char** cmndRight, int i, int i2, int totalBytes){
	int pipein[2];
	int pipeout[2];
	char read_msg[totalBytes];
	int summation = 0;
	if (pipe(pipein)) {
        	printf("pipe");
        	exit(0);
    	}
    	if (pipe(pipeout)) {
        	printf("pipe");
        	exit(0);
    	}
    	pid_t child0 = 0, child1 = 0;
    	child0 = fork();
    	if(child0 != 0)
    		child1 = fork();
	if(child0< 0 || child1 < 0){
        	printf("Fork failed\n");
        	exit(0);
	}else if(child0 == 0){
		close(pipeout[1]);// second pipe will not be used
		close(pipeout[0]);
        	close(pipein[0]);
        	dup2(pipein[1], 1); //pipe management
        	close(pipein[1]);
       		execvp(cmndLeft[0], cmndLeft);
       		printf("Cannot execute\n");
       		exit(0);
        }else{//Parent and child1
        		if(child1 == 0){		
        			close(pipein[0]);
				close(pipein[1]);//second children will not use first pipe so close it
				close(pipeout[1]);
			}
        	if(child1 > 0){///Parent
			close(pipein[1]); // close for parent
			close(pipeout[0]);
			int bytes;
			int readCall = 0, writeCall = 0;
			while ((bytes = read (pipein[0], read_msg, sizeof (read_msg))) > 0) {
				readCall += 1;
            			if (write (pipeout[1], read_msg, bytes) != bytes) {//may write return -1 
					break;
            			}
				writeCall += 1;
				summation += bytes;
			}
			close(pipein[0]);
			close(pipeout[1]);
			int status;
			waitpid(child1, &status, 0); //make the parent wait for child to end
			printf("\ncharacter count: %d\n", summation);
			printf("read-call count: %d\n", readCall);
			printf("write-call count: %d\n", writeCall);
		}else{
			dup2(pipeout[0], 0);
			close(pipeout[0]);
	     		execvp(cmndRight[0], cmndRight);
       		printf("asdsdExec failed\n");
       		
		}
        }
}
/*
void timeS(int mode, int bytes, int lmt){
	char asd[12];
	char* cmnd[3] = {"./producer", "500000", (char*)NULL};
	char* cmnd2[3] = {"./consumer", "500000", (char*)NULL};
	double times[lmt];
	for(int i = 0; i < lmt; i++){
		//usleep(900000);
		struct timeval current_time, end_time;
		gettimeofday(&current_time, NULL);
		if(mode == 1)
			pipedExecution(cmnd, cmnd2, 2, 2);
		else{
			tappedMode(cmnd, cmnd2, 2, 2, bytes);
		}
		gettimeofday(&end_time, NULL);
		//printf(" total time: %f seconds\n", (double)((double)(current_time.tv_usec - starting)/(double)1000000));
		times[i] = (double)((double)(end_time.tv_usec - current_time.tv_usec)/(double)1000000)  + (double)(end_time.tv_sec - current_time.tv_sec);
	//	printf(" total time: %f seconds\n", times[i]);
	}
	double TT = 0;
	for(int i = 0; i < lmt; i++){
		printf("%f\n", times[i]);
		if(times[i] > 0)
			TT += times[i];
	}
	TT = (double)(TT/lmt);
	printf("\nresult: %f\n", TT);
}*/

int main(int argc, char *argv[])
{
	char* string = "" , *pipes[2];
	char** cmnds;
	char** cmndsRight;
	int communication_mode = atoi(argv[2]), bytes = atoi(argv[1]);
	int piped= 0, noArgs, noArgsRight;
	printf("%s\n", "\033[H\033[J");
	printf("" COLOR);
	/*sleep(1);  // just to give it a visual taste
	printf("\n***Furbro shell***\n");
	sleep(1);
	printf("\n***Syntax: for pipeless commands, default syntax: \"ps aux\" ***\n");
	sleep(1);
	printf("\n***Syntax: for piped commands, default syntax: \"ls | sort\" ***\n\n");*/
	while(1){
		cmnds = malloc(sizeof(char*)*120);
		cmndsRight = malloc(sizeof(char*)*128);
		piped = 0;
		pipes[1] = NULL; // to clean the second string in pipes so the command from older piped execution wont stored again
		noArgs = 0;
		noArgsRight = 0;
		if(input(&string) != 1)
			continue;
        	piped = checkPipe(string, pipes);
        	printf("\n");
        	if(piped == 0){
			divideSpaces(string, cmnds,&noArgs);
			if(strcmp(cmnds[0], "exit") == 0)
				break;
			/*if(strcmp(cmnds[0], "times") == 0){    for time mesuarements
        			timeS(communication_mode, bytes, atoi(cmnds[1]));
				continue;
			}
			struct timeval current_time;	for timing 
			gettimeofday(&current_time, NULL); 
			int starting = current_time.tv_usec;*/
			pipelessExecution(cmnds, noArgs);
			/*gettimeofday(&current_time, NULL);
			printf(" total time: %f seconds\n", (double)((double)(current_time.tv_usec - starting)/(double)1000000));*/
			
		}else{
			divideSpaces(pipes[0], cmnds,&noArgs);
			divideSpaces(pipes[1], cmndsRight,&noArgsRight);
			
		/*	struct timeval current_time;
			gettimeofday(&current_time, NULL);	for timing
			int starting = current_time.tv_usec;*/
			if(communication_mode == 1)
				pipedExecution(cmnds, cmndsRight, noArgs, noArgsRight);
			else{
				tappedMode(cmnds, cmndsRight, noArgs, noArgsRight, bytes);
			}
			/*gettimeofday(&current_time, NULL);
			printf(" total time: %f seconds\n", (double)((double)(current_time.tv_usec - starting)/(double)1000000));*/
		}
		free(cmnds);
		free(cmndsRight);	
	}
	free(cmnds);
	free(cmndsRight);
	printf("Exitting shel...\n");	
}
