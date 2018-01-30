#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/stat.h>
#include <fcntl.h>


int sig_flag = 0;

/*
 * Command_Cd() is used when the cd command is entered into the shell. It takes an array of arrays containing the command entered and any arguments
 * entered. It checks if a directory has been specified and if not it will redirect to the home directory. The current working directory is printed at the
 * end of the function.
 */

void command_cd(char **args) {
    char cwd[100];
    char *home = getenv("HOME");

    if(*(args+1) == NULL) {
        if(chdir(home) != 0) {
            perror("Error ");
            return;
        }
    }
    else if(chdir(*(args+1)) != 0) {
        perror("Error ");
        return;
    }
    getcwd(cwd, 100);
    printf("%s\n", cwd);
}


/*
 * Command is used to execute all commands given except for cd. A fork() is used to create a child process which will run the command entered.
 * It takes in an argument of an array of arrays which contain the command and its arguments. If the fork is successful and the command has been
 * run successfully it will wait until the child process has finished before returning.
 */

void command(char **args) {
    pid_t pid;
    int status;

    if((pid = fork()) < 0) {
        printf("Error fork failed\n");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        if(execvp(args[0], args) < 0) {
            printf("%s: Command not found\n", args[0]);
            exit(EXIT_FAILURE);
        }
    }
    else {
        while(wait(&status) != pid);
    }
}

/*
 * This function is used to seperate the line of input from the console into its seperate components. The components are seperated by spaces.
 */

int command_args(char *input, char **args_in) {
    char *token = NULL;
    int count=0, marker=-1;

    strtok(input, "\n");
    token = strtok(input, " ");
    while(token != NULL) {
        if(strcmp(token, ">")==0) {
            marker = count;
        }
        args_in[count] = token;
        count++;
        token = strtok(NULL, " ");
    }
    free(token);
    return marker;
}

/*
 * Basic signal handling function. Used for catching the SIGINT signal. Stops the program from exiting when a SIGINT signal is seen.
 */

void sig_handler(int signo) {
    if(signo == SIGINT) {
        fflush(stdout);
        sig_flag = 1;
    }
}

int main(int argc, char *argv[]) {
    char *temp = NULL;
    char **args;
    size_t len = 0;
    time_t usr_time;
    struct tm *info;
    char time_buffer[50];
    char **command_buffer = malloc(10*sizeof(char*));;
    int i=0, read, redirect_mark, file = 0, cur_dup = 0;

    signal(SIGINT, sig_handler);
    fflush(stdout);
    for(i=0; i<10; i++) {
        *(command_buffer+i) = NULL;
    }

    while(1) {
        args = malloc(10*sizeof(char*));
        for(i=0; i<10; i++) {
            *(args+i) = NULL;
        }
        time(&usr_time);
        info = localtime(&usr_time);
        strftime(time_buffer, 50, "[%d/%m %H:%M]", info);
        printf("%s # ", time_buffer);
        read = (int) (getline(&temp, &len, stdin));
        if(read == EOF) {
            free(args);
            free(temp);
            for(i=0; i<10; i++) {
                if(*(command_buffer+i) != NULL) {
                    free(*(command_buffer+i));
                }
            }
            free(command_buffer);
            exit(EXIT_SUCCESS);
        }
        else if(strcmp(temp, "\n") ==0) {
            free(args);
            continue;
        }
        strtok(temp, "\n");
        if(sig_flag != 0) {
            sig_flag = 0;
            free(args);
            continue;
        }
        redirect_mark = command_args(temp, args);
        if(redirect_mark != -1) {
            cur_dup = dup(1);
            redirect_mark++;
            file = open(*(args+redirect_mark), O_CREAT|O_TRUNC|O_RDWR, S_IRWXU);
            dup2(file, 1);
            *(args+redirect_mark--) = NULL;
            *(args+redirect_mark) = NULL;
            redirect_mark = -1;
        } else {
            redirect_mark = 0;
        }
        if(strcmp((args[0]), "cd") == 0) {
            command_cd(args);
        }
        else {
            command(args);
        }
        if(redirect_mark == -1) {
            dup2(cur_dup, 1);
            close(file);
        }
        free(args);
    }
}
