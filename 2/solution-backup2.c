#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>


//Added libs
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

#include <string.h>
/*
Given errors:
echo "\
"

Assertion failed: (t->size > 0), function token_strdup, file parser.c, line 39.
*/

// Execute the command
//printf("debug: %d\n", cmd->arg_count);
//for (uint32_t i = 0; i < cmd->arg_count; ++i)
//	printf("debug: %s\n", cmd->args[i]);
//cmd->args[sizeof(cmd->args) / sizeof(cmd->args[0]) - 1] = NULL;

//Нужно как то безопасно добавлять NULL 
//command_append может



//struct command cmd2;
//cmd2.exe = "ls";
//char *args[] = {"ls", NULL};
//cmd2.args = args;
//cmd2.arg_count = 2;
//cmd2.arg_capacity = 2;

static void
execute_command(struct command *cmd){
	pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(1);
	} else if (pid == 0) {  // Child process
        
		
		char *combined_args[cmd->arg_count + 2]; // +1 for the function name, +1 for the NULL
    	combined_args[0] = cmd->exe;
		for (int i = 0; i < cmd->arg_count; i++) {
			combined_args[i + 1] = cmd->args[i];
		}

		combined_args[cmd->arg_count + 1] = NULL;
        execvp(combined_args[0], combined_args);
		//execvp(cmd2.exe, cmd2.args);
        perror("execvp");
        exit(1);
    } else {  // Parent process
        int status;
        waitpid(pid, &status, 0);
    }
}
// можно делать redirect в ребенке а можно в родителе, но лучше в ребенке

//int original_stdout = dup(STDOUT_FILENO); // Save the original stdout
//int fd = open(line->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
// if (fd < 0) {
// 	perror("open");
// 	exit(1);
// }
// // Redirect stdout to the opened file descriptor
// dup2(fd, STDOUT_FILENO);
// close(fd);
//Redirect back
// dup2(original_stdout, STDOUT_FILENO);
// close(original_stdout);

static void
execute_command_line(const struct command_line *line)
{
	/*
	MY CODE
	*/

	const struct expr *e = line->head;
	execute_command(&line->head->cmd);
	
	
	
	/* REPLACE THIS CODE WITH ACTUAL COMMAND EXECUTION */
	/*
	assert(line != NULL);
	printf("================================\n");
	printf("Command line:\n");
	printf("Is background: %d\n", (int)line->is_background);
	printf("Output: ");
	if (line->out_type == OUTPUT_TYPE_STDOUT) {
		printf("stdout\n");
	} else if (line->out_type == OUTPUT_TYPE_FILE_NEW) {
		printf("new file - \"%s\"\n", line->out_file);
	} else if (line->out_type == OUTPUT_TYPE_FILE_APPEND) {
		printf("append file - \"%s\"\n", line->out_file);
	} else {
		assert(false);
	}
	printf("Expressions:\n");
	const struct expr *e = line->head;
	while (e != NULL) {
		if (e->type == EXPR_TYPE_COMMAND) {
			printf("\tCommand: %s", e->cmd.exe);
			for (uint32_t i = 0; i < e->cmd.arg_count; ++i)
				printf(" %s", e->cmd.args[i]);
			printf("\n");
		} else if (e->type == EXPR_TYPE_PIPE) {
			printf("\tPIPE\n");
		} else if (e->type == EXPR_TYPE_AND) {
			printf("\tAND\n");
		} else if (e->type == EXPR_TYPE_OR) {
			printf("\tOR\n");
		} else {
			assert(false);
		}
		e = e->next;
	}
	*/
}

// struct command {
// 	char *exe;
// 	char** args;
// 	uint32_t arg_count;
// 	uint32_t arg_capacity;
// };

// enum expr_type {
// 	EXPR_TYPE_COMMAND,
// 	EXPR_TYPE_PIPE,
// 	EXPR_TYPE_AND,
// 	EXPR_TYPE_OR,
// };

// struct expr {
// 	enum expr_type type;
// 	/** Valid if the type is COMMAND. */
// 	struct command cmd;
// 	struct expr *next;
// };

// enum output_type {
// 	OUTPUT_TYPE_STDOUT,
// 	OUTPUT_TYPE_FILE_NEW,
// 	OUTPUT_TYPE_FILE_APPEND,
// };

// struct command_line {
// 	struct expr *head;
// 	struct expr *tail;
// 	enum output_type out_type;
// 	/** Valid if the out type is FILE. */
// 	char *out_file;
// 	bool is_background;
// };

static void
see_command_line(const struct command_line *line){
	struct expr *e = line->head;

	//Type of output
	if (line->out_type == OUTPUT_TYPE_STDOUT) {
		printf("Output type: stdout\n");
	} else if (line->out_type == OUTPUT_TYPE_FILE_NEW) {
		printf("Output type: new file\n");//, line->out_file);
	} else if (line->out_type == OUTPUT_TYPE_FILE_APPEND) {
		printf("Output type: append file\n");//, line->out_file);
	} else {
		assert(false);
	}

	//Is it background
	printf("Background: %d\n", (int)line->is_background);

	//Expressions
	while (e != NULL) {
		printf("New expression:\n");
		if (e->type == EXPR_TYPE_COMMAND) {
			printf("\tCommand: %s \n", e->cmd.exe);
			for (uint32_t i = 0; i < e->cmd.arg_count; ++i)
				printf("\targ: %s\n", e->cmd.args[i]);
			//printf("\n");
		} else if (e->type == EXPR_TYPE_PIPE) {
			printf("\tPIPE\n");
		} else if (e->type == EXPR_TYPE_AND) {
			printf("\tAND\n");
		} else if (e->type == EXPR_TYPE_OR) {
			printf("\tOR\n");
		} else {
			assert(false);
		}
		e = e->next;
	}
}

int main_withonepipe() {
    int pipe_fd[2];

    if (pipe(pipe_fd) == -1) {
        perror("pipe");
        exit(EXIT_FAILURE);
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
        exit(EXIT_FAILURE);
    } else if (pid == 0) {  // Child process
        close(pipe_fd[0]); // Close the read end

        // Redirect stdout to the write end of the pipe
        dup2(pipe_fd[1], STDOUT_FILENO);
        close(pipe_fd[1]);

        execlp("pwd", "pwd", NULL); // Execute the 'pwd' command
        perror("execlp");
        exit(EXIT_FAILURE);
    } else {  // Parent process
        close(pipe_fd[1]); // Close the write end

        char buffer[256];
        ssize_t bytes_read = read(pipe_fd[0], buffer, sizeof(buffer));
        close(pipe_fd[0]);

        if (bytes_read > 0) {
            buffer[bytes_read] = '\0'; // Null-terminate the string
            printf("Child process (pwd) output: %s", buffer);
        } else {
            printf("Child process did not produce any output.");
        }
    }

    return 0;
}

int main() {
	//echo "123456789" | tail -c 5 | tail -c 2
	//вот это допустим будем запускать

	struct command cmd_echo;
	cmd_echo.exe = "echo";
	char *args1[] = {"ls", "-l", NULL};
	cmd_echo.args = args1;// {"echo", "123456789", NULL};
	cmd_echo.arg_count = 2;
	cmd_echo.arg_capacity = 2;

	struct command cmd_tail1;
	cmd_tail1.exe = "tail";
	char *args2[] = {"tail", "-c", "5", NULL};
	cmd_tail1.args = args2;
	cmd_tail1.arg_count = 3;
	cmd_tail1.arg_capacity = 3;

	struct command cmd_tail2;
	cmd_tail2.exe = "tail";
	char *args3[] = {"tail", "-c", "2", NULL};
	cmd_tail2.args = args3;
	cmd_tail2.arg_count = 3;
	cmd_tail2.arg_capacity = 3;
	
	struct expr expr_echo;
	expr_echo.type = EXPR_TYPE_COMMAND;
	expr_echo.cmd = cmd_echo;
	expr_echo.next = NULL;
	
	struct expr pipe1;
	pipe1.type = EXPR_TYPE_PIPE;
	expr_echo.next = NULL;

	struct expr expr_tail1;
	expr_tail1.type = EXPR_TYPE_COMMAND;
	expr_tail1.cmd = cmd_tail1;
	expr_tail1.next = NULL;

	struct expr pipe2;
	pipe2.type = EXPR_TYPE_PIPE;
	expr_echo.next = NULL;
	
	struct expr expr_tail2;
	expr_tail2.type = EXPR_TYPE_COMMAND;
	expr_tail2.cmd = cmd_tail2;
	expr_tail2.next = NULL;

	expr_echo.next = &pipe1;
	pipe1.next = &expr_tail1;
	expr_tail1.next = &pipe2;
	pipe2.next = &expr_tail2;
	//expr_tail1.next = &expr_tail2;
	struct command_line cmdline;
		
	cmdline.head = &expr_echo;  // The head of the pipeline
	cmdline.tail = &expr_tail2;  // The tail of the pipeline
	cmdline.out_type = OUTPUT_TYPE_STDOUT;  // Output type is STDOUT
	cmdline.out_file = NULL;  // No output file specified
	cmdline.is_background = false;  // Not running in the background

	
	struct expr *e = cmdline.head;
	struct expr *prev_e = NULL;
	int prev_fd[2];

	while (e != NULL) {
		//printf("New command:\n");
		if (e->type == EXPR_TYPE_COMMAND) {
			if (e->next != NULL && e->next->type == EXPR_TYPE_PIPE){
				printf("Pipe will be after %s\n", e->cmd.exe);
				prev_e = e;
			} else if (e->next == NULL){
				printf("Last command %s\n", e->cmd.exe);
			}

			
		} else if (e->type == EXPR_TYPE_PIPE) {
			printf("\tPIPE\n");

			int fd[2];
            if (pipe(fd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

			pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
			}

            if (pid == 0) {  // Child process
				

                close(fd[0]);
                dup2(fd[1], STDOUT_FILENO);
				close(fd[1]);

                execlp("ls","ls", "-l", NULL);
                perror("execlp");
                exit(EXIT_FAILURE);
            } else {  // Parent process
                close(fd[1]);
				char buffer[1024];
				ssize_t num_bytes;

				// Read data from the read end of the pipe
				num_bytes = read(fd[0], buffer, sizeof(buffer));
				if (num_bytes < 0) {
					perror("read");
					exit(EXIT_FAILURE);
				}

				// Null-terminate the received data to print it as a string
				buffer[num_bytes] = '\0';

				// Print the message received from the child process
				printf("Parent received: %s", buffer);
				wait(NULL); // Wait for the child process to finish
            }

			
		} else if (e->type == EXPR_TYPE_AND) {
			printf("\tAND\n");
		} else if (e->type == EXPR_TYPE_OR) {
			printf("\tOR\n");
		} else {
			assert(false);
		}
		e = e->next;
	}

	printf("\nEND OF PROGRAM\n");
	/*
    const char *commands[] = {"ls", "PIPE", "grep", "PIPE", "wc", "-l"};
    int num_commands = sizeof(commands) / sizeof(commands[0]);

    //int prev_pipe[2] = {-1, -1};

    for (int i = 0; i < num_commands; i++) {
        if (strcmp(commands[i], "PIPE") == 0) {
            // This is a pipe separator, set up a new pipe
            int pipe_fd[2];
            if (pipe(pipe_fd) == -1) {
                perror("pipe");
                exit(EXIT_FAILURE);
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("fork");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {  // Child process
                if (i > 0) {
                    close(prev_pipe[1]); // Close the write end of the previous pipe
                    dup2(prev_pipe[0], STDIN_FILENO); // Redirect stdin to the read end of the previous pipe
                    close(prev_pipe[0]);
                }

                close(pipe_fd[0]); // Close the read end of the current pipe
                dup2(pipe_fd[1], STDOUT_FILENO); // Redirect stdout to the write end of the current pipe
                close(pipe_fd[1]);

                execlp(commands[i + 1], commands[i + 1], NULL);
                perror("execlp");
                exit(EXIT_FAILURE);
            } else {  // Parent process
                if (prev_pipe[0] != -1) {
                    close(prev_pipe[0]);
                    close(prev_pipe[1]);
                }
                prev_pipe[0] = pipe_fd[0];
                prev_pipe[1] = pipe_fd[1];
            }
            i++; // Skip the "PIPE" command
        }
    }

    if (prev_pipe[1] != -1) {
        close(prev_pipe[1]);
    }

    // Parent process should read the final output
    char buffer[256];
    ssize_t bytes_read = read(prev_pipe[0], buffer, sizeof(buffer));
    close(prev_pipe[0]);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("Final output: %s", buffer);
    }
	*/
    return 0;
}

int
main_test(void)
{
	struct command cmd;
    cmd.exe = "ls";
    char *args[] = {"ls", "-l", NULL};
    cmd.args = args;
    cmd.arg_count = 2;
    cmd.arg_capacity = 2;

	execute_command(&cmd);
	return 0;
}

int
main_old(void)
{
	const size_t buf_size = 1024;
	
	//buffer to feed parser
	char buf[buf_size];
	
	//number of bytes read
	int rc;

	//creating new parser
	struct parser *p = parser_new();

	//while there is something to read read
	while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0) {
		//каждый раз заново кормит и проходит по всему чтобы распарсить
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
			see_command_line(line);
			printf("\n\n");
			command_line_delete(line);
		}
	}
	parser_delete(p);
	return 0;
}

int
main_m(void)
{
	const size_t buf_size = 1024;
	
	//buffer to feed parser
	char buf[buf_size];
	
	//number of bytes read
	int rc;

	//creating new parser
	struct parser *p = parser_new();

	//while there is something to read read
	while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0) {
		//каждый раз заново кормит и проходит по всему чтобы распарсить
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
			execute_command_line(line);
			command_line_delete(line);
		}
	}
	parser_delete(p);
	return 0;
}
