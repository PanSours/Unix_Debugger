#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <sys/ptrace.h>
#include <sys/reg.h>
#include <sys/stat.h>
#include <sys/syscall.h>
#include <fcntl.h>
#include <linux/user.h>

/*global inter-process variables*/
pid_t pidp = 0;         //used for signal handling
pid_t pidc = -1;        //used for signal handling

int main (int argc, char *argv[]) {
    
        /*string arrays used for temporary storage of different strings*/
        char *pch;
    	char *str;
	char buffer[80];
        char buffer2[2];
    	char split[10][80];

    	int counter,counter2,i,limit_value = 0;
    	int terminate = 0;      // used for knowing when we hit go or not
    	int exitgo = 0;         // used for knowing when we hit quit or not

        /*flags that are used by all commands of the CLI to know which command we have finally*/
    	int flag_trace = 0;
	int flag_mode = 0;
	int flag_limit = 0;
 	int flag_redirect1 = 0;
 	int flag_redirect2 = 0;
 	int flag_redirect3 = 0;
 	int flag_go = 0;
	int goexo;
        
        /*temporary arrays to implement go with arguments*/
	char split2[10][80];
	char *pch2;
	char **goarg;   //this array is given to execvp
        
        /*variables used for tracing and counting only once the signals from a program*/
	int flag_exec = 0;
	int flag_call = 0;
	int scalls = 0;
	
        int status;     //for waiting on fork
        pid_t pid;      //this is the process being forked to operate the tracing
    	long orig_eax;  //value of the register
        
        int blocking = 0;       //to implement the blocking-mode we use this flag

        /*pointers to 3 different files to implement the redirect command*/
	FILE *fp;
	FILE *fp2;
        FILE *fp3;
        
        /*these arrays keep the names of the files for each category of command redirect*/
	char stdinp[80];
        char stdoutp[80];
        char stderror[80];
        
        pid_t pid2;     //process to be forked when redirect occurs
        int fd[2];      //pipe
        
        /*prototypes of the signal handling functions*/
        void signal_handler_sighup (int sigtype);
        void signal_handler_sigint (int sigtype);
        void signal_handler_sigterm (int sigtype);
        
        pidp = getpid();    //using it in signal handling
        
        /*functions for signal handling, they call the function of the second parameter to handle the signal happening on the first,when it occurs*/
        signal (SIGHUP,signal_handler_sighup);
        signal (SIGINT,signal_handler_sigint);
        signal (SIGTERM,signal_handler_sigterm);
        
        /*check if the pipe was successful*/
        if (pipe(fd) == -1) {
                perror("Pipe");
                exit(EXIT_FAILURE);
        }
        
        /*check if we gave arguments to our program*/
        if (argc != 2) {
                perror("No Location Was Given!!");
		exit(EXIT_FAILURE);
	}

	counter2 = 0;

	for (i=0;i<10;i++) {
        	split2[i][0] = (int)NULL;
    	}
        
        /*the following routine splits the path we gave as argument to picodb to take the last string which is the name of the program we want to trace */
	pch2 = strtok(argv[1], "/");

	while (pch2 != NULL) {
                strcpy((char*)&split2[counter2][0], pch2);
		counter2++;
       	 	pch2 = strtok(NULL, "/");
    	}
        
        /*main loop of the picodb, when it turns to 1 the program exits,happens when we press quit*/
	while (exitgo == 0) {

        	terminate = 0;
                
                /*secondary loop, of the CLI, when it turns to 1 the program continues to the trace routine,happens when we press go*/
                while (terminate == 0) {
                    
                        /*we read the command typed by the user*/
                        str = gets(buffer);
                        
			if (str == NULL) {
				perror("Could Not Read Command");
				exit(EXIT_FAILURE);
        		}
			else {
				counter = 0;
                                
                                /*this routine is the same as the split with /, now we are splitting the line we read with whitespace to know on one command how many arguments we have and which*/
				for (i=0;i<10;i++) {
                			split[i][0] = (int)NULL;
        			}

				pch = strtok (str, " ");

            			while (pch != NULL) {
					strcpy((char*)&split[counter][0], pch);
					counter++;
                			pch = strtok(NULL, " ");
            			}
        		}
                        
                        /*after we have split the line we red, we check to see if the command is valid*/
                        if ((strcmp ((char*)&split[0][0], "redirect") == 0)||(strcmp((char*)&split[0][0], "r") == 0)) { //if we have redirect or r

				if ((strcmp ((char*)&split[1][0], "stdin")) == 0) {                     //if next have stdin
			  		if (split[3][0] != '\0') {                                      //if after filename we have sth it means too many arguments
                    				printf("Too Many Arguments on redirect\n");
					}
					else if ((split[2][0] != '\0') && (split[3][0] == '\0')) {      //if we have stdin and a filename

						fp = fopen((char*)&split[2][0],"r");                    //we check if file exists
						if (fp == NULL) {
							printf("Error Opening the File, Make Sure the File Exists\n");
						}
						else {
                            				flag_redirect1 = 1;                             //if it exists we keep the name of the file to stdinp
                            				strcpy(stdinp,(char*)&split[2][0]);
							fclose(fp);
						}
					}
					else {
						printf ("Less Arguments, You Need to Enter a Filename\n");
					}
				}
				else if ((strcmp ((char*)&split[1][0], "stdout")) == 0) {               //same as above

					if (split[3][0] != '\0') {
                    				printf("Too Many Arguments on redirect\n");
					}
					else if ((split[2][0] != '\0') && (split[3][0] == '\0')) {
                        			flag_redirect2 = 1;
                        			strcpy(stdoutp,(char*)&split[2][0]);
                                                fp2 = fopen (stdoutp,"w");
					}
					else {
						printf ("Less Arguments, You Need to Enter a Filename\n");
					}
				}
				else if ((strcmp ((char*)&split[1][0], "stderr")) == 0) {               //same as above

					if (split[3][0] != '\0') {
                    				printf("Too Many Arguments on redirect\n");
					}
					else if ((split[2][0] != '\0') && (split[3][0] == '\0')) {
                    				flag_redirect3 = 1;
                        			strcpy(stderror,(char*)&split[2][0]);
                                                fp3 = fopen (stderror,"w");
					}
					else {
						printf ("Less Arguments, You Need to Enter a Filename\n");
					}
				}
				else {  //if we dont find stdin,stdout,stderr wrong call on redirect
                    			printf("Invalid Call on redirect,Type help to See redirect Options\n");
				}
			}
			else if ((strcmp ((char*)&split[0][0], "trace") == 0)||(strcmp((char*)&split[0][0], "t") == 0)) {       //if we find trace or t

			  	if ((strcmp ((char*)&split[1][0], "file-management") == 0 )&& (split[2][0] == '\0')) {          //if after follows f.management

					flag_trace = 1;
				}
				else if ((strcmp ((char*)&split[1][0], "process-control") == 0 )&& (split[2][0] == '\0')) {     //if after follows p.control
                                        
                                        flag_trace = 2;
				}
				else if ((strcmp ((char*)&split[1][0], "all") == 0 )&& (split[2][0] == '\0')) {                 //if after follows all
                                        
                                        flag_trace = 3;
				}
				else {  //else wrong
                    			printf("Too Many Arguments/Wrong Arguments on Treace\n");
				}
			}
			else if ((strcmp ((char*)&split[0][0], "blocking-mode") == 0)||(strcmp((char*)&split[0][0], "b") == 0)) {       //if we find b-mode or b

			  	if (split[1][0] == '\0') { //if follows empty arguments, less arguments
					printf("Less Arguments on blocking-mode\n");
				}
				else if ((strcmp ((char*)&split[1][0], "on") == 0 )&& (split[2][0] == '\0')) {  //if on

					flag_mode = 1;
				}
				else if ((strcmp ((char*)&split[1][0], "off") == 0 )&& (split[2][0] == '\0')) { //if off

					flag_mode = 2;
				}
				else {  //else either we have more arguments or wrong ones
                    			printf("Too Many Arguments/Wrong Arguments on blocking-mode\n");
				}
			}
			else if ((strcmp ((char*)&split[0][0], "limit-trace") == 0)||(strcmp((char*)&split[0][0], "l") == 0)) { //if we find limit-trace  or l

				if (split[1][0] != '\0') { //if follows another argument

					if ((strcmp ((char*)&split[1][0], "0") == 0)) { //if it is 0
						printf("0 Is Not a Valid Limit, Give a Number or Nothing to Trace all Processes\n");
					}
					else {  //if not  we keep the value in limit_value
						if (split[2][0] == '\0') {
							flag_limit = 1;
							limit_value = atoi((char*)&split[1][0]);
						}
						else {
							printf("Too Many Arguments on limit-trace\n");
						}
					}
				}
				else { //if we have only limit-trace or l without arguments
					flag_limit = 1;
					limit_value = 777;      //we  use 777 just to know that we have no limit
				}
			}
			else if ((strcmp ((char*)&split[0][0], "go") == 0)||(strcmp((char*)&split[0][0], "g") == 0)) {  //if we find go or g
                            
                                /*we use goexo as flag to see if the user before typing go has types limit-trace, b-mode and trace commands which are needed*/
				goexo = 0;
				if (flag_trace == 0) {
					printf("Please Give trace mode before Tracing\n");
					goexo = 1;
				}

				if (flag_limit == 0) {
					printf("Please Give limit-trace before Tracing\n");
					goexo = 1;
				}

				if (flag_mode == 0) {
					printf("Please Give trace mode before Tracing\n");
					goexo = 1;
				}

				if (goexo == 0) {       //if the user has given all required commands before typing go

					if (split[1][0] == '\0') {      //if go is alone with no argument
						
                                                flag_go = 1;
						terminate = 1;
                                                /*we set up goarg with the name of the program to be traces and a NULL*/
						goarg = malloc(2 * sizeof(char *));
						goarg[0] = malloc((strlen(split2[counter2-1])+ 1) * sizeof(char));
						strcpy(goarg[0],split2[counter2-1]);
						goarg[1]=NULL;
					}
					else {  //if go comes with arguments
						flag_go = 2;
						terminate = 1;
                                                /*we set goarg with the arguments except the program name and NULL*/
                        			goarg = malloc(((counter-1)+2)*sizeof(char *));
                        			goarg[0] = malloc((strlen(split2[counter2-1]) + 1) * sizeof(char));
                        			strcpy(goarg[0],split2[counter2-1]);

						for(i=1;i<=(counter-1);i++){
                                                        goarg[i] = malloc((strlen((char*)&split[i][0]) + 1) * sizeof(char));
							strcpy(goarg[i],(char*)&split[i][0]);
						}

						goarg[counter]=NULL;
					}
				}
				else {
					printf ("You Have to Enter all Required Commands, for Help Press help/h\n");
				}
			}
			else if ((strcmp ((char*)&split[0][0], "quit") == 0)||(strcmp((char*)&split[0][0], "q") == 0)) { //if we find quit or q

			  	if (split[1][0] == '\0') {      //if it has no arguments we change the termination flags
					terminate = 1;
			  		exitgo = 1;
			  	}
			  	else {  //if it comes with arguments its wrong
					printf ("Quit Can't Have Arguments, Please Try Again\n");
			  	}
			}
			else if ((strcmp ((char*)&split[0][0], "help") == 0)||(strcmp((char*)&split[0][0], "h") == 0)) {        //help has same handling as quit

			  	if(split[1][0] == '\0') {

 					printf(" \n");
					printf("------------------------------COMMAND MENU------------------------------\n");
					printf(" \n");

					printf("Definition: trace/t <category>, traces the category of <category> sys calls\n");
					printf("<category>:\n");
					printf("'process-control', traces the specific category of sys calls\n");
					printf("'file-management', traces the specific category of sys calls\n");
					printf("'all', traces both categories\n");
					printf(" \n");

					printf("Definition: redirect/r  <stream> <filename>, redirects the <stream> into of from <filename>\n");
					printf("<stream>:\n");
					printf("'stdin', the program takes input from <filename>\n");
					printf("'stdout', changes the standard output to <filename>\n");
					printf("'stderr', changes the standard error output to <filename>\n");
					printf(" \n");

					printf("Definition: blocking-mode/b  <mode>, sets the <mode>\n");
					printf("<mode>:\n");
					printf("'on', stops the program every time a system call is being traced and asks th user\n");
					printf("if he wants to stop tracing or not, y is for yes, n for no\n");
					printf("'off', the program runs without any interrupts\n");
					printf(" \n");

					printf("Definition: limit-trace/l  <number>, sets a limit on how many processes will be traced\n");
					printf("<number>:\n");
					printf(" \n");

     					printf("Definition: help/h, prints the command line menu\n");
					printf(" \n");

					printf("Definition: quit/q, exits picodb\n");
					printf(" \n");

					printf("Definition: go/g, the program starts running loaded with the commands we entered\n");
					printf(" \n");
          			}
          			else {
					printf("Help Can't Have Arguments, Please Try Again\n");
    				}
  			}
			else {  //in any other case the command is unknown
				printf("Unknown Command, Please Make Sure your Command is Correct, Type help/h for Help\n");
			}
		}
                /*we dont want the trace routine to proceed if we have hit quit so if exitgo == 0 we proceed*/
                if (exitgo == 0) {
                    
			/* New process */
                        if ((pid = fork()) == -1) {
                                perror("Fork");
                                exit(EXIT_FAILURE);
                        }
                        
                        pidc = pid;     //we keep the value of the child pid to use it in signal handling
                        
                        if (pid == 0) { //child
                            
                                /*functions to take the signals when occur and run the default function of them inside the child's process*/
                                signal (SIGHUP,SIG_DFL);
                                signal (SIGINT,SIG_DFL);
                                signal (SIGTERM,SIG_DFL);

                                ptrace(PTRACE_TRACEME,pid,NULL,NULL);   //updates the kernel to keep a memory space for this process 

				if (flag_redirect1 == 1) {      //if we have redirect stdin
                                    
					/*we make a fork and we read from file through out the pipe and give the program what we red */
					if ((pid2 = fork()) == -1) {
                                		perror("Fork");
                                		exit(EXIT_FAILURE);
                        		}

					if (pid2 == 0) {
						char buf[128];
						int filefd;
						close(fd[0]);
  						filefd = open(stdinp,O_RDONLY);
  						while(read(filefd,buf,sizeof(buf)) > 0) {
   							write(fd[1],buf,sizeof(buf));
   							memset(buf,0,sizeof(buf));
  						}
  						close(fd[1]);
						close(filefd);
						exit(EXIT_SUCCESS);
					}
					else {
						close(fd[1]);
						dup2(fd[0],0);  //redirect stdinput to pipe read
        					close(fd[0]);
						execvp(argv[1], goarg);
					}				
				}

				if (flag_redirect2 == 1) {      //if we have redirect stdout
                                    
					/*we make a fork and we redirect stdout to pipe write*/
					if ((pid2 = fork()) == -1) {
                                		perror("Fork");
                                		exit(EXIT_FAILURE);
                        		}

					if (pid2 == 0) {
						char buf[128];
						close(fd[1]);
  						while(read(fd[0],buf,sizeof(buf)) > 0)
   						fprintf (fp2,"%s",buf);
						close(fd[0]);
						exit(EXIT_SUCCESS);
					}
					else {
						close(fd[0]);
						dup2(fd[1],1);  //redirect stdoutput to pipe write
        					close(fd[1]);
						fclose(fp2);
						execvp(argv[1], goarg);
					}				
				}

				if (flag_redirect3 == 1) {      //if we have redirect stderr
					/*we make a fork and we redirect stderr to pipe write*/
					if ((pid2 = fork()) == -1) {
                                		perror("Fork");
                                		exit(EXIT_FAILURE);
                        		}

					if (pid2 == 0) {
						char buf[128];
						close(fd[1]);
  						while(read(fd[0],buf,sizeof(buf)) > 0)
   						fprintf (fp3,"%s",buf);
						close(fd[0]);
						exit(EXIT_SUCCESS);
					}
					else {
						close(fd[0]);
						dup2(fd[1],2);  //redirect stderror to pipe write
        					close(fd[1]);
						fclose(fp3);
						execvp(argv[1], goarg);
					}				
				}
                                /*if we didnt hit any redirect command before go we just execute the exec here */
				if (flag_redirect1 != 1 && flag_redirect2 != 1 && flag_redirect3 != 1) {
					execvp(argv[1], goarg);				
				}
                        }
                        else { //parent

                                flag_exec = 0;
                                flag_call = 0;
                                scalls = 0;
                            
                                while (1) {
                                    
                                        if (blocking == 1) {    //if we have pressed n on blocking-mode question to continue tracing the tracing routine breaks
                                            blocking = 0;
				            kill(pid,SIGKILL);	//we kill the child process 
                                            break;
                                        }
                                        
                                        wait (&status);
                                        /*wait for child process to exit on exec*/
                                        if (WIFEXITED(status)) {
                                                break;
                                        }
                                        
                                        if (flag_exec == 1) {   //if exec happened we are ready to count the syscall

                                                orig_eax = ptrace(PTRACE_PEEKUSER, pid, 4 * ORIG_EAX, NULL);    //we load the orig with the value of the register, the value has the syscall just caused

                                                if (flag_trace == 1) {  //if trace is file-management

                                                        if (orig_eax == SYS_execve || orig_eax == SYS_fork || orig_eax == SYS_wait4 || orig_eax == SYS_kill) {
                                                                if (flag_call == 0) {   //if we havent count the call count it and change the flag so we dont count it again
                                                                        flag_call = 1;
                                                                        scalls++;
                                                                }
                                                                else {
                                                                        if (flag_call == 1) {   //if this is 1it means the sys call has been counted and now it goes to exit so after this time a new sycall will arrive

                                                                                flag_call=0;

                                                                                if (flag_mode == 1) { //blocking-mode on

                                                                                        char *responce;
                                                                                        int flag_resp = 0;

                                                                                        printf("Proceed to Next Trace?\n");
                                                                                        while (flag_resp == 0){
                                                                                                    responce = gets(buffer2);
                                                                                                    if (strcmp (responce, "n") == 0) {
                                                                                                            blocking = 1;
                                                                                                            flag_resp = 1;
                                                                                                    }
                                                                                                    else if (strcmp (responce, "y") == 0){
                                                                                                            flag_resp = 1;
                                                                                                    }
                                                                                                    else {
                                                                                                            printf ("Please Type y or n\n");
                                                                                                    }
                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                        }

                                                        if ( flag_call == 1 ) {

                                                                if (limit_value == 777) {
                                                                        printf("Found System Call %d \n",orig_eax);
                                                                }
                                                                else if (scalls <= limit_value) {
                                                                        printf("Found System  Call %d \n",orig_eax);
                                                                }
                                                                else {
                                                                        flag_call = -1;
                                                                }
                                                        }
                                                }
                                                else if (flag_trace == 2) {     //same as above

                                                        if (orig_eax == SYS_open || orig_eax == SYS_close || orig_eax == SYS_read || orig_eax == SYS_write) {
                                                                if (flag_call == 0) {
                                                                        flag_call = 1;
                                                                        scalls++;
                                                                }
                                                                else {
                                                                        if (flag_call == 1) {

                                                                                flag_call=0;
                                                                                
                                                                                if (flag_mode == 1) {

                                                                                        char *responce;
                                                                                        int flag_resp = 0;

                                                                                        printf("Proceed to Next Trace?\n");
                                                                                        while (flag_resp == 0){
                                                                                                    responce = gets(buffer2);
                                                                                                    if (strcmp (responce, "n") == 0) {
                                                                                                            blocking = 1;
                                                                                                            flag_resp = 1;
                                                                                                    }
                                                                                                    else if (strcmp (responce, "y") == 0){
                                                                                                            flag_resp = 1;
                                                                                                    }
                                                                                                    else {
                                                                                                            printf ("Please Type y or n\n");
                                                                                                    }
                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                        }

                                                        if ( flag_call == 1 ) {

                                                                if (limit_value == 777) {
                                                                        printf("Found System Call %d \n",orig_eax);
                                                                }
                                                                else if (scalls <= limit_value) {
                                                                        printf("Found System  Call %d \n",orig_eax);
                                                                }
                                                                else {
                                                                        flag_call = -1;
                                                                }
                                                        }
                                                }
                                                else if (flag_trace == 3) {     //same as above

                                                        if (orig_eax == SYS_execve || orig_eax == SYS_fork || orig_eax == SYS_wait4 || orig_eax == SYS_kill || orig_eax == SYS_open || orig_eax == SYS_close || orig_eax == SYS_read || orig_eax == SYS_write) {
                                                                if (flag_call == 0) {
                                                                        flag_call = 1;
                                                                        scalls++;
                                                                }
                                                                else {
                                                                        if (flag_call == 1) {

                                                                                flag_call=0;
                                                                                
                                                                                if (flag_mode == 1) {

                                                                                        char *responce;
                                                                                        int flag_resp = 0;

                                                                                        printf("Proceed to Next Trace?\n");
                                                                                        while (flag_resp == 0){
                                                                                                    responce = gets(buffer2);
                                                                                                    if (strcmp (responce, "n") == 0) {
                                                                                                            blocking = 1;
                                                                                                            flag_resp = 1;
                                                                                                    }
                                                                                                    else if (strcmp (responce, "y") == 0){
                                                                                                            flag_resp = 1;
                                                                                                    }
                                                                                                    else {
                                                                                                            printf ("Please Type y or n\n");
                                                                                                    }
                                                                                        }
                                                                                }
                                                                        }
                                                                }
                                                        }

                                                        if ( flag_call == 1 ) {

                                                                if (limit_value == 777) {
                                                                        printf("Found System Call %d \n",orig_eax);
                                                                }
                                                                else if (scalls <= limit_value) {
                                                                        printf("Found System  Call %d \n",orig_eax);
                                                                }
                                                                else {
                                                                        flag_call = -1;
                                                                }
                                                        }
                                                }
                                        }
                                        else {
                                                flag_exec = 1;
                                        }

                                        ptrace(PTRACE_SYSCALL,pid,NULL,NULL);
                                }
                        }
		}
        }
	printf ("I Traced %d System Calls\n",scalls);
        /*free resources*/
        if (flag_go == 1) {
                free(goarg[0]);
		free(goarg);
	}
	else if (flag_go == 2) {
                for (i=0;i<counter;i++) {
                        free(goarg[i] );
		}
                free(goarg);
	}
	return 0;
}

/*implementation of signal handling functions*/
void signal_handler_sighup (int sigtype) {

	if (pidc == -1) {
            
		kill(pidp,SIGKILL);     //if the is no child at the signal arrival we kill the parent
                
	}
	else {
            
                kill(pidc,SIGKILL);     //we kill the child
                kill(pidp,SIGKILL);     //we kill the parent too
	}
        
}

void signal_handler_sigint (int sigtype) {

	if (pidc == -1) {
            
		kill(pidp,SIGKILL);	//if the is no child at the signal arrival we kill the parent
                
	}
	else {
            
                kill(pidc,SIGKILL);     //we kill the child
                kill(pidp,SIGKILL);     //we kill the parent too
	}

}


void signal_handler_sigterm (int sigtype) {

	if (pidc == -1) {
            
		kill(pidp,SIGKILL);	//if the is no child at the signal arrival we kill the parent
                
	}
	else {
            
                kill(pidc,SIGKILL);     //we kill the child
                kill(pidp,SIGKILL);     //we kill the parent too
	}
        
}
