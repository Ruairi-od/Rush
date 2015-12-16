#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <stdio_ext.h>


int sig_flag = 0;//Flag used to signal catch of SIGINT Signal

/*
 * Command_Cd() is used when the cd command is entered into the shell. It takes an array of arrays containing the command entered and any arguments
 * entered. It checks if a directory has been specified and if not it will redirect to the home directory. The current working directory is printed at the
 * end of the function.
 */

void command_cd(char **args)
{
	int return_val;
	char cwd[100];
	char *home = getenv("HOME");

	if(*(args+1) == NULL)//Check if a directory has been specified
	{
		if((return_val = chdir(home)) != 0)//Change directory to home directory.
		{
			perror("Error ");
			return;

		}
	}
	else if((return_val = chdir(*(args+1))) != 0)//Change directory to directory specified.
	{
		perror("Error ");
		return;
	}
	getcwd(cwd, 100);//Get current working directory.
	printf("%s\n", cwd);//Printf current working directory.
}


/*
 * Command is used to execute all commands given except for cd. A fork() is used to create a child process which will run the command entered.
 * It takes in an argument of an array of arrays which contain the command and its arguments. If the fork is successful and the command has been
 * run successfully it will wait until the child process has finished before returning.
 */

void command(char **args)
{
	pid_t pid;
	int status;

	if((pid = fork()) < 0)//Fork failed
	{
		printf("Error fork failed\n");
		exit(EXIT_FAILURE);
	}
	else if (pid == 0)   //child process
	{
		if(execvp(args[0], args) < 0)//Check if command executed correctly
		{
			printf("Error execvp failed\n");
			exit(EXIT_FAILURE);
		}
	}
	else   //Parent process
	{
		while(wait(&status) != pid);//Wait until the process has completed before finishing.
	}
}

/*
 * This function is used to seperate the line of input from the console into its seperate components. The components are seperated by spaces.
 */

int command_args(char *input, char **args_in)
{
	char *token = NULL;
	int count=0, marker=-1;

	strtok(input, "\n");//The end of line marker is trimmed off the input.
	token = strtok(input, " ");//The first token would be the command entered.
	while(token != NULL)//Function is null terminated.
	{
		if(strcmp(token, ">")==0)//Check if the input contains a ">" symbol. This is used in main to output to a file instead of stdout.
		{
			marker = count;//A marker flag is set to the location of the < symbol
		}
		args_in[count] = token;//The seperate components are assigned to an array.
		count++;
		token = strtok(NULL, " ");
	}
	free(token);//Memory free'd
	return marker;
}

/*
 * Basic signal handling function. Used for catching the SIGINT signal. Stops the program from exiting when a SIGINT signal is seen.
 */

void sig_handler(int signo)
{
	if(signo == SIGINT)
	{
		fflush(stdout);
		sig_flag = 1;//Sets the signal flag to show it is not a command so should not be run in main.
	}
}

void command_history(char *input, char **buffer, int *count)
{
	*count = *count % 10;
	buffer[*count] = malloc(strlen(input)+1);
	memcpy(buffer[*count], input, strlen(input)+1);
	*count = *count + 1;
}

int main(int argc, char *argv[])
{
	char *temp = NULL, *temp_2 = NULL;
	char **args;
	size_t len = 0;
	time_t usr_time;
	struct tm *info;
	char time_buffer[50];
	char ch_1=NULL, ch_2=NULL, ch_3=NULL;
	char **command_buffer = malloc(10*sizeof(char*));;
	int i=0, read, redirect_mark, file, cur_dup, command_count=0;

	signal(SIGINT, sig_handler);//Catching the signal.
	fflush(stdout);
	for(i=0; i<10; i++)
	{
		*(command_buffer+i) = NULL;//Arrays are initialised.
	}

	while(1)
	{
		args = malloc(10*sizeof(char*));//memory allocated for commands and arguments.
		for(i=0; i<10; i++)
		{
			*(args+i) = NULL;//Arrays are initialised.
		}
		time(&usr_time);
		info = localtime(&usr_time);//getting time off the local system.
		strftime(time_buffer, 50, "[%d/%m %H:%M]", info);//Time is got and printed before the command prompt.
		printf("%s # ", time_buffer);
		read = (getline(&temp, &len, stdin));//Attempt to read line of input from console.
		temp_2 = malloc(strlen(temp));
		strcpy(temp_2, temp);
		temp = malloc((strlen(temp)+1) * sizeof(char));
		temp[0] = ch_1;
		for(i=0;i<=strlen(temp_2);i++)
		{
			temp[i+1] = temp_2[i];
		}
		free(temp_2);
		if(read == EOF)//If EOF signal is recieved
		{
			free(args);//Free memory
			free(temp);
			for(i=0; i<10; i++)
			{
				if(*(command_buffer+i) != NULL)
				{
					free(*(command_buffer+i));
				}
			}
			free(command_buffer);
			exit(EXIT_SUCCESS);//Exit
		}
		else if(strcmp(temp, "\n") ==0)//If input was empty
		{
			free(args);//free memory
			continue;//Dont continue with current loop.
		}
		strtok(temp, "\n");//The trailing end line character is trimmed form the input.
		if(sig_flag != 0)//If a signal was caught
		{
			sig_flag = 0;
			free(args);//Free memory
			continue;//Dont continue with current loop.
		}
		command_history(temp, command_buffer, &command_count);
		redirect_mark = command_args(temp, args);//Call function to split input into seperate function and arguments
		if(redirect_mark != -1)//If redirect has been requested
		{
			cur_dup = dup(1);//Save curret output to terminal location
			redirect_mark++;
			file = open(*(args+redirect_mark), O_CREAT|O_TRUNC|O_RDWR, S_IRWXU);//Open file to output stdout to with permissions for owner.
			dup2(file, 1);//Redirect stdout to file location.
			*(args+redirect_mark--) = NULL;
			*(args+redirect_mark) = NULL;
			redirect_mark = -1;

		} else
		{
			redirect_mark = 0;
		}
		if(strcmp((args[0]), "cd") == 0)//Check if cd command has been entered.
			command_cd(args);//Call function to execute cd command
		else
			command(args);//Call function to execute all other commands.
		if(redirect_mark == -1)//If stdout was redirected
		{
			dup2(cur_dup, 1);//Reset stdout to the terminal
			close(file);//Close the file the was opened.
		}
		free(args);//Free memory
	}
}
