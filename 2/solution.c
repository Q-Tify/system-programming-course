#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>



//Added libraries
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
#include <string.h>
#include <errno.h>



int isFileDescriptorClosed(int fd) {
    return fcntl(fd, F_GETFD) == -1;
}

static int
change_dir(char *destinationDir){
	char currentDirectory[PATH_MAX];

	if (getcwd(currentDirectory, sizeof(currentDirectory)) == NULL) {
		perror("getcwd");
		return 1;
	}

	char fullPath[PATH_MAX];
	snprintf(fullPath, sizeof(fullPath), "%s/%s", currentDirectory, destinationDir);
	
	if (chdir(fullPath) != 0) {
		perror("chdir");
		return 1;
	}
	
	return 0;
}

static char **
get_program_name_with_args(struct command cmd){
	// +1 for the function name, +1 for the NULL
	char **combined_args = malloc((cmd.arg_count + 2) * sizeof(char *));

    combined_args[0] = cmd.exe;
    for (uint32_t i = 0; i < cmd.arg_count; i++) {
        combined_args[i + 1] = cmd.args[i];
    }
    combined_args[cmd.arg_count + 1] = NULL;

    return combined_args;
}



static int
execute_command_line(struct command_line *line, struct parser *p)
{
	assert(line != NULL);
    
    //Counting the number of commands in line
    struct expr *e_for_count = line->head;
    int number_of_commands = 0;
	while (e_for_count != NULL) {
		if (e_for_count->type == EXPR_TYPE_COMMAND) {
            number_of_commands++;
        }
        e_for_count = e_for_count->next;
    }

    //Creating pipes for commands
    int pipes[number_of_commands - 1][2];
    for (int i = 0; i < number_of_commands - 1; i++) {
        if (pipe(pipes[i]) == -1) {
            perror("pipe");
            exit(1);
        }
    }

    pid_t child_pid;
    int i_pipe = 0;
	const struct expr *e = line->head;
    char **combined_args = NULL;

	while (e != NULL) {
		if (e->type == EXPR_TYPE_COMMAND) {
            if(strcmp("cd", e->cmd.exe) == 0){
                //Check for null
                change_dir(e->cmd.args[0]);
            } else {
                if ((child_pid = fork()) == -1) {
                    perror("fork");
                    exit(1);
                }

                if (child_pid == 0) {
                    if (e != line->head) {
                        // Redirect stdin from the previous pipe
                        dup2(pipes[i_pipe - 1][0], STDIN_FILENO);
                        close(pipes[i_pipe - 1][0]);
                        close(pipes[i_pipe - 1][1]);
                    }

                    if (e->next != NULL) {
                        // Redirect stdout to the next pipe
                        dup2(pipes[i_pipe][1], STDOUT_FILENO);
                        close(pipes[i_pipe][0]);
                        close(pipes[i_pipe][1]);
                    } else {
                        if (line->out_type == OUTPUT_TYPE_STDOUT) {

                        } else if (line->out_type == OUTPUT_TYPE_FILE_NEW) {
                            int fd_out = open(line->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
                            if (fd_out == -1) {
                                perror("open");
                                exit(1);
                            }
                            dup2(fd_out, STDOUT_FILENO);
                            close(fd_out);
                        } else if (line->out_type == OUTPUT_TYPE_FILE_APPEND) {
                            int fd_out = open(line->out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
                            if (fd_out == -1) {
                                perror("open");
                                exit(1);
                            }
                            dup2(fd_out, STDOUT_FILENO);
                            close(fd_out);
                        } else {
                            assert(false);
                        }
                    }

                    //Closing all other unused pipes
                    for (int i = 0; i < number_of_commands - 1; i++) {
                        close(pipes[i][0]);
                        close(pipes[i][1]);
                    }
                    
                    if (strcmp("exit", e->cmd.exe) == 0){
                        if(e->cmd.arg_count == 0){
                            parser_delete(p);
                            command_line_delete(line);
                            exit(EXIT_SUCCESS);
                        } else {
                            int some_x = atoi(e->cmd.args[0]);
                            parser_delete(p);
                            command_line_delete(line);
                            exit(some_x);
                        }
                    }else{
                        combined_args = get_program_name_with_args(e->cmd);
                        execvp(combined_args[0], combined_args);
                        perror("execvp");
                        exit(EXIT_FAILURE);
                    }
                }
            }
            i_pipe++;
		} else if (e->type == EXPR_TYPE_PIPE) {
			//printf("\tPIPE\n");
		} else if (e->type == EXPR_TYPE_AND) {
			//printf("\tAND\n");
		} else if (e->type == EXPR_TYPE_OR) {
			//printf("\tOR\n");
		} else {
			assert(false);
		}
		e = e->next;
	}

    // Close all pipe ends in the parent process
    for (int i = 0; i < number_of_commands - 1; i++) {
        close(pipes[i][0]);
        close(pipes[i][1]);
    }

    int status;
    pid_t pid;
    int max_pid = -1;
    int last_exit_status;

    while ((pid = wait(&status)) != -1 || errno != ECHILD){
        int last_child_exit_status = WEXITSTATUS(status);
        if ((int)pid > max_pid){
            max_pid = (int)pid;
            last_exit_status = last_child_exit_status;
        }
        //printf("CHILD %d with status %d\n", (int)pid, last_child_exit_status);
    }

    return last_exit_status;
}


int 
is_exit(struct command_line *line){
    //If there is more than one command
    if (line->head->next != NULL){
        return -1;
    }

    //If it is not exit
    if (line->head->type == EXPR_TYPE_COMMAND) {
        if (strcmp(line->head->cmd.exe, "exit") != 0){
            return -1;
        }
    }

	struct expr *e = line->head;
    if(e->cmd.arg_count == 0){
        return EXIT_SUCCESS;
    } else {
        return atoi(e->cmd.args[0]);
    }

    
    return -1;
}



int
main(void)
{
    int exit_code = 0;
	const size_t buf_size = 1024;
	char buf[buf_size];
	int rc;
	struct parser *p = parser_new();
	while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0) {
		parser_feed(p, buf, rc);
		struct command_line *line = NULL;
		while (true) {
			enum parser_error err = parser_pop_next(p, &line);
			if (err == PARSER_ERR_NONE && line == NULL)
				break;
			if (err != PARSER_ERR_NONE) {
				printf("Error: %d\n", (int)err);
				continue;
			}

            //If exit is alone in line
            if((exit_code = is_exit(line)) != -1){
                command_line_delete(line);
                parser_delete(p);
                exit(exit_code);
            }

            exit_code = execute_command_line(line, p);

			command_line_delete(line);
		}
	}
	parser_delete(p);

	return exit_code;
}