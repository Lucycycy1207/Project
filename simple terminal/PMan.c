#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>


int comm_format(char* split, int format, char cm[]);


int comm_format(char* split, int format, char cm[]) {
	if (strcmp(split, "bglist") == 0)
		strcpy(cm, "bglist");
		//printf("bglist\n");
	
	else if (strcmp(split, "bgkill") == 0)
		//printf("bgkill\n");
		strcpy(cm, "bgkill");

	else if (strcmp(split, "pstatn") == 0)
		//printf("pstat\n");
		strcpy(cm, "pstatn");

	else if (strcmp(split, "bgstop") == 0)
		//printf("bgstop\n");
		strcpy(cm, "bgstop");

	else if (strcmp(split, "bgstart") == 0)
		//printf("bgstart\n");
		strcpy(cm, "bgstart");

	else if (strcmp(split, "bg") == 0)
		//printf("bg\n");
		strcpy(cm, "bg");

	else {
		//printf("bad command");	
		format = 0;
		cm = "";
	}
	return format;
	
}

int main(int argc, char* argv[])
{

	pid_t pid;          // The ID of child process
	char cm[10];
	char arg[100];
	char* args[66];



	while (1) {
		int format = 1;
		char* split;
		printf("PMan: >");

		char command[100];
		char copy[100];
		fgets(command, 100, stdin);

		command[strcspn(command, "\n")] = '\0';

		if (strcmp(command, "") == 0) {
			printf("empty\n");
			continue;
		}
		const char s[2] = " ";

		strcpy(copy, command);
		split = strtok(command, s);//this will change the command value

		format = comm_format(split, format, cm);
		//printf("format:%d\n", format);
		if (format == 0) {
			char error_message[] = "PMan: > ";
			strcat(error_message, command);
			strcat(error_message, ": command not found\n");
			//perror(error_message);
			printf("%s", error_message);
		}
		else {//format is correct
			//cm has command

			//check arg format for bglist
			if (strcmp(cm, "bglist") == 0) {
				if (strcmp(copy, cm) != 0) {
					//bglist args num is wrong
					printf("bglist args num is wrong\n");
					continue;
				}
			}


			if (strcmp(cm, "bglist") != 0)
				split = strtok(NULL, s);//the arg



			if (split == NULL && strcmp(cm, "bglist") != 0) {
				//besides bglist, args number wrong
				//bglist but it has args
				printf("%s args num is wrong\n", cm);
				continue;
			}
			else {

				if (strcmp(cm, "bg") == 0) {
					int i = 0;
					//strcpy(arg, split);
					//printf("split:%s\n", split);
					while (split != NULL) {
						//printf("split:%s\n", split);
						//strcpy(args[i], split);
						
						//printf("args[%d]:%s\n", i, split);
						//i++;
						//printf("hi\n");
						split = strtok(NULL, s);
						//strcpy(args[i++], arg);
					}
					//char* x = malloc(strlen(yo) + 1);
					//strcpy(whaddup, yo);
					//strcpy(args[i], split);

						
						
					
				}
				else {
					strcpy(arg, split);
				}

				//printf("arg: %s\n", arg);
				//printf("cm: %s\n", cm);
				//printf("args[0]%s\n", args[0]);
				//printf("args[1]%s\n", args[1]);
				//printf("args[2]%s\n", args[2]);
				//printf("args[3]%s\n", args[3]);
				
				if (execvp(args[0], args) < 0) {
					printf("error");
				}
				strcat(arg, "&");
				//printf("arg: %s\n", arg);
				
				//args num and format is correct
				
				if (strcmp(cm, "bg") == 0) {
					bg_entry(arg);

				}

				else if (strcmp(cm, "bglist") == 0) {
					if (execvp("ps", args) < 0) {
						perror("Error on execvp");
					}
				}

				exit(EXIT_SUCCESS);
				//bgkill; kill pid
				//bgstop;kill -STOP pid
				//bgstart <pid>

				else if (pid > 0) {
					//parent process
					// store information of the background child process in your data structures

				}
				else {
					// Fail to create new process
					perror("\nFail to create a new process.\n");
					exit(EXIT_FAILURE);
				}
				


			}

		}


	}
}
	
void bg_entry(char* args) {
	
	pid_t pid;
	pid = fork();
	if (pid == 0) {
		if (execvp("bg", args) < 0) {
			perror("Error on execvp");
		}
		exit(EXIT_SUCCESS);
	}
	else if (pid > 0) {
		// store information of the background child process in your data structures
	}
	else {
		perror("fork failed");
		exit(EXIT_FAILURE);
	}
}


void check_zombieProcess(void) {
	int status;
	int retVal = 0;

	while (1) {
		usleep(1000);
		if (headPnode == NULL) {
			return;
		}
		retVal = waitpid(-1, &status, WNOHANG);
		if (retVal > 0) {
			//remove the background process from your data structure
		}
		else if (retVal == 0) {
			break;
		}
		else {
			perror("waitpid failed");
			exit(EXIT_FAILURE);
		}
	}
	return;
}


	/*
	comm_format(command);
	*/
	

	//print sth in a interval
	/*
	if (argc != 3) {
		fprintf(stderr, "Usage: inf tag interval\n");
	}
	else {
		const char* tag = argv[1];
		int interval = atoi(argv[2]);
		while (1) {
			printf("%s\n", tag);
			sleep(interval);
		}
	}
	*/


