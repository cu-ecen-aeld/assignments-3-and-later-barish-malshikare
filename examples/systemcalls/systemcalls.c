#include "systemcalls.h"

/**
 * @param cmd the command to execute with system()
 * @return true if the command in @param cmd was executed
 *   successfully using the system() call, false if an error occurred,
 *   either in invocation of the system() call, or if a non-zero return
 *   value was returned by the command issued in @param cmd.
 */
bool do_system(const char *cmd)
{

		/*
		 *   Call the system() function with the command set in the cmd
		 *   and return a boolean true if the system() call completed with success
		 *   or false() if it returned a failure
		 */
		int rc = system(cmd);
		if(rc==-1)
				return false;	
		return true;
}

/**
 * @param count -The numbers of variables passed to the function. The variables are command to execute.
 *   followed by arguments to pass to the command
 *   Since exec() does not perform path expansion, the command to execute needs
 *   to be an absolute path.
 * @param ... - A list of 1 or more arguments after the @param count argument.
 *   The first is always the full path to the command to execute with execv()
 *   The remaining arguments are a list of arguments to pass to the command in execv()
 * @return true if the command @param ... with arguments @param arguments were executed successfully
 *   using the execv() call, false if an error occurred, either in invocation of the
 *   fork, waitpid, or execv() command, or if a non-zero return value was returned
 *   by the command issued in @param arguments with the specified arguments.
 */

bool do_exec(int count, ...)
{
		va_list args;
		va_start(args, count);
		char * command[count+1];
		int i;
		for(i=0; i<count; i++)
		{
				command[i] = va_arg(args, char *);
		}
		command[count] = NULL;

		pid_t new_id = fork();
		int status;
		switch(new_id)
		{
				case 0: //child process
						int err= execv(command[0],&command[0]);
						if(err == -1){
								printf("failed to execv: %d\n",err);
								exit (1);
						}
						break;
				case -1:
						printf("failed to create process\n");
						return 0;
				default: printf("parent process\n");
						 break;
		}

		wait(&status);
		if (WIFEXITED(status)) {
				if (WEXITSTATUS(status) == 0) {
						return 1;
				} else {
						return 0;
				}
		}
		else {
				return 0;
		} 
		va_end(args);
}

/**
 * @param outputfile - The full path to the file to write with command output.
 *   This file will be closed at completion of the function call.
 * All other parameters, see do_exec above
 */
bool do_exec_redirect(const char *outputfile, int count, ...)
{
		va_list args;
		va_start(args, count);
		char * command[count+1];
		int i;
		for(i=0; i<count; i++)
		{
				command[i] = va_arg(args, char *);
		}
		command[count] = NULL;
		int kidpid;
		int fd = open(outputfile, O_WRONLY|O_TRUNC|O_CREAT, 0644);
		if (fd < 0) { 
				printf("Error in opening file: %d\n",fd);
		}
		switch (kidpid = fork()) {
				case -1: perror("fork");
				case 0:
						 // Redirect stdout to the file
						 if (dup2(fd, STDOUT_FILENO) < 0) {
								 perror("dup2 failed\n");
								 close(fd);
								 exit(1);
						 }
						 if(-1 == execvp(command[0], &command[0])) {
								 printf("Error in execvp");
						 }
						 break;
				default:
						 int status;
						 wait(&status);
						 break;
		}
		va_end(args);
		printf("Exitnig redirect\n");
		return true;
}
