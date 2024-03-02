/*
project: 01
author: Aiden Bauer
email: abauer2@umbc.edu
student id: IW51818
description: a simple linux shell designed to perform basic linux commands
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <ctype.h>
#include "utils.h"

// DEFINE THE FUNCTION PROTOTYPES
void user_prompt_loop();
void get_user_command();
char** parse_command();
int execute_command();
int whitespace(int, char*);


int main(int argc, char **argv)
{
    /*
    Write the main function that checks the number of argument passed to ensure 
    no command-line arguments are passed; if the number of argument is greater 
    than 1 then exit the shell with a message to stderr and return value of 1
    otherwise call the user_prompt_loop() function to get user input repeatedly 
    until the user enters the "exit" command.
    */

    if (argc > 1) {
        fprintf(stderr, "Detected command line arguments; Cannot accept command line arguments. \n");
        return 1;
    }
    user_prompt_loop();
    return 0;
}

/*
user_prompt_loop():
Get the user input using a loop until the user exits, prompting the user for a command.
Gets command and sends it to a parser, then compares the first element to the two
different commands ("/proc", and "exit"). If it's none of the commands, 
send it to the execute_command() function. If the user decides to exit, then exit 0 or exit 
with the user given value. 
*/
void user_prompt_loop()
{

    /*
    loop:
        1. prompt the user to type command by printing >>
        2. get the user input using get_user_command() function 
        3. parse the user input using parse_command() function 
        Example: 
            user input: "ls -la"
            parsed output: ["ls", "-la", NULL]
        4. compare the first element of the parsed output to "/proc"and "exit"
        5. if the first element is "/proc" then you have the open the /proc file system 
           to read from it
            i) concat the full command:
                Ex: user input >>/proc /process_id_no/status
                    concated output: /proc/process_id_no/status
            ii) read from the file line by line. you may user fopen() and getline() functions
            iii) display the following information according to the user input from /proc
                a) Get the cpu information if the input is /proc/cpuinfo
                - Cpu Mhz
                - Cache size
                - Cpu cores
                - Address sizes
                b) Get the number of currently running processes from /proc/loadavg
                c) Get how many seconds your box has been up, and how many seconds it has been idle from /proc/uptime
                d) Get the following information from /proc/process_id_no/status
                - the vm size of the virtual memory allocated the vbox 
                - the most memory used vmpeak 
                - the process state
                - the parent pid
                - the number of threads
                - number of voluntary context switches
                - number of involuntary context switches
                e) display the list of environment variables from /proc/process_id_no/environ
                f) display the performance information if the user input is /proc/process_id_no/sched
        6. if the first element is "exit" the use the exit() function to terminate the program
        7. otherwise pass the parsed command to execute_command() function 
        8. free the allocated memory using the free() function
    */
    int end_flag = 0;
    while (end_flag > -1) {
        int parsed_size = 0;
        char** parsed = parse_command(&parsed_size);

        // if just null, go next
        if (!parsed || !parsed[0]) {
            end_flag = 0;
            continue;
        }
        // implementation for exit
        if (strcmp(parsed[0], "exit") == 0) {
            int e_id = 0; // 0 is default exit
            int valid = 1;

            // check if there is an exit code
            if (parsed[1]) {
                int ctr = 0;

                // check if the exit code is a valid number by checking if any digit is a letter
                // misusing shell builtins (providing invalid argument) defaults to exit code 2
                // invalid arguments to exit give exit code 128
                while (ctr != (int) strlen(parsed[1]) - 1) {
                    if (!isdigit(parsed[1][ctr])) {
                        valid = 0;
                        break;
                    }
                    ctr++;
                }
            }

            // if argument passed is valid, update e_id
            if (valid) {
                if (parsed[1])
                    e_id = atoi(parsed[1]);
            }
            // free allocated memory
            for (int i = 0; i < parsed_size; i++) {
                free(parsed[i]);
                parsed[i] = NULL;
            }
            free(parsed);
            parsed = NULL;
            
            // if valid id, then exit, otherwise continue prompting
            if (valid)
                exit(e_id);
            continue;

        // implementation for proc
        // i was told you can do either proc or /proc, so /proc only
        } else if (strcmp(parsed[0], "/proc") == 0) {
            //concat strings
            char* ccat = NULL;
            if (parsed[1]) {
                for (int i = 1; i < parsed_size - 1; i++) {
                    ccat = strcat(parsed[0], parsed[i]);
                }
            }
            // read from file
            char data;
            FILE* fp = fopen(ccat, "r");
            if (NULL == fp) {
                printf("%s: No such file or directory. \n", parsed[0]);
            } else {
                do {
                    data = fgetc(fp);
                    printf("%c", data);
                } while(data != EOF);

                fclose(fp);
            }
            end_flag = 0;
        // pass other arguments to execute command
        } else {
            end_flag = execute_command(parsed, parsed_size);
        }

        // free parsed array, including each string individually first
        for (int i = 0; i < parsed_size; i++) {
            free(parsed[i]);
            parsed[i] = NULL;
        }
        free(parsed);
        parsed = NULL;
    }
}

/*
get_user_command():
Take input of arbitrary size from the user and return to the user_prompt_loop()
*/
void get_user_command(char** buf, size_t size, size_t* characters)
{
    // initially allocate a set size for the buffer
    char* result = (char* ) malloc(size * sizeof(char));   
    if (result == NULL) {
        perror("Unable to allocate buffer");
        exit (1);
    }

    printf("$ ");
    // getline automatically reallocates space despite reading more than the size, the actual size gets put in characters
    *characters = getline(&result, &size, stdin);
    *buf = result;
    //*buf = (char*) realloc(buf, *characters);
}

/*
parse_command():
Take command grabbed from the user and parse appropriately.
Example: 
    user input: "ls -la"
    parsed output: ["ls", "-la", NULL]
Example: 
    user input: "echo     hello                     world  "
    parsed output: ["echo", "hello", "world", NULL]
*/
char** parse_command(int* arr_size)
{
    // initialize buffer for reading user input
    char* buffer = NULL;
    size_t bufsize = 80;
    size_t charlen; 

    // read user input, remove escape sequences for parsing
    get_user_command(&buffer, bufsize, &charlen);
    //buffer = unescape(buffer, stderr); // * THIS LEAKS MEMORY!!!! *

    // variables necessary to track for while loop
    int cur_size = -1;
    int tot_size = whitespace(0, buffer); // check for leading whitespace and skip it

    // get size of array
    while (cur_size != 0) {
        cur_size = first_unquoted_space(&buffer[tot_size]);
        tot_size += cur_size;
        // check for and skip any whitespace
        tot_size = whitespace(tot_size, buffer);
        *arr_size = *arr_size + 1;
    }
    
    // dynamically allocate parsed array of strings using arr_size and size of buffer
    // only allocate if characters/strings exist within buffer
    char **parsed_arr = NULL;
    if (*arr_size > 1) {
        parsed_arr = malloc((*arr_size) * sizeof(char*) + 1);
        for (int i = 0; i < *arr_size - 1; i++) {
            parsed_arr[i] = malloc(charlen * sizeof(char) + 1); 
            //parsed_arr[i] = NULL;
        }
    } else {
        free(buffer);
        return NULL;
    }

    // reinitialize counters etc.
    int counter = 0;
    cur_size = 0;
    tot_size = whitespace(0, buffer); // check for leading whitespace and skip it

    // loop again with the array size, adding strings to the parse array
    while (counter <= *arr_size) {
        cur_size = first_unquoted_space(&buffer[tot_size]);

        // set spots in parsed_arr to strings within buffer
        for (int i = 0; i < cur_size; i++) {
            parsed_arr[counter][i] = buffer[tot_size + i];
        }

        // use unescape on individual strings before adding end of string character
        // this does not solve unescape leaking but if this section were commented out I would have 0 leaks in my code
        if (counter && counter < *arr_size) {
            parsed_arr[counter - 1] = unescape(parsed_arr[counter - 1], stderr);
        }

        // add end of string character to each string
        if (cur_size)
            parsed_arr[counter][cur_size] = '\0';
        tot_size += cur_size;

        // check for and skip any whitespace
        tot_size = whitespace(tot_size, buffer);
        counter++;
    }

    // set last spot of array to NULL
    //parsed_arr[*arr_size - 1] = NULL;

    // free the orig string
    free(buffer);
    buffer = NULL;

    return parsed_arr;
}

/*
execute_command():
Execute the parsed command if the commands are neither /proc nor exit;
fork a process and execute the parsed command inside the child process
*/
int execute_command(char** parsed, int size)
{
    int pid = fork();

    // pid < 0 is a fork failure
    if (pid < 0) {
        fprintf(stderr, "Fork Failed");
        return 1;
    // pid == 0 is standard execution
    } else if (pid == 0) {
        int status_code = execvp(parsed[0], parsed);
        // free the dangling process before force-exiting it
        if (status_code == -1) {
            fprintf(stderr, "%s: command not found\n", parsed[0]);
            for (int i = 0; i < size; i++) {
                free(parsed[i]);
            }
            free(parsed);
            exit(0);
        }
    // wait on child process
    } else {
        waitpid(pid, NULL, 0);
    }

    return 0;

}

/*
whitespace():
Counts the amount of whitespace between letters within a string.
Helper function for parse_command()

num: current position in the entire string
buffer: string to iterate through
*/
int whitespace(int num, char* buffer) {
    int ii = 0;
    // if there is any whitespace, just keep iterating through string until a letter is found
    while (ii > -1) {
        if (buffer[num + ii] == '\0') {
            return -1;
        } else if (buffer[num + ii] == ' ') {
            ii++;
        } else {
            num += ii;
            ii = -1;
        }
    }
    return num;
}