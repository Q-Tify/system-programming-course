#include "parser.h"

#include <assert.h>
#include <stdio.h>
#include <unistd.h>


//Added libs
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <limits.h>
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
		for (uint32_t i = 0; i < cmd->arg_count; i++) {
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




/// ПРЕДЛАГАЮ ВЫНЕСТИ CD В ОТДЕЛЬНУЮ ФУНКЦИЮ КОТОРАЯ ВЕРНЕТ INT

//ПОТОМ ПОЙТИ РЕАЛИЗОВЫВАТЬ ДОБАВЛЕНИЕ В ФАЙЛ ТЕКСТА

//ПОТОМ РЕАЛИЗОВАТЬ EXIT
/*Команда 'exit' тоже специальная как 'cd'. Потому что влияет на
  сам терминал. Ее нужно реализовать вручную. Но учтите, что она
  затрагивает терминал только если exit - единственная команда в
  строке. 'exit' - завершить терминал. 'exit | echo 100' - не
  завершать терминал, выполнить этот exit как любую другую команду
  через exec-функции.*/

// потом - Код должен собираться успешно с данными флагами компилятора:
  //`-Wextra -Werror -Wall -Wno-gnu-folding-constant`.

//  - +5 баллов: поддержка операторов && и ||.

//- +5 баллов: поддержка &.

//Проверить что pipe закрывается и когда одна команда и когда несколько команд 
//и когда в середине запись в файл а потом еще какая то команда выполняется

//Сделать чтение и вывод из буфера неограниченным

//Нужно проверить что в центре нет какого то вывода в файл в пайпе иначе хз как это
//должно выполняться
static void 
change_directory(char *to_directory){
	char currentDirectory[PATH_MAX];

	if (getcwd(currentDirectory, sizeof(currentDirectory)) == NULL) {
		perror("getcwd");
		//return 1;
	}

	char fullPath[PATH_MAX];
	snprintf(fullPath, sizeof(fullPath), "%s/%s", currentDirectory, to_directory);

	if (chdir(fullPath) != 0) {
		perror("chdir");
		//return 1;
	}

	//return 0;
}

static void
execute_command_line(const struct command_line *line)
{
	int original_stdout = dup(STDOUT_FILENO);
	int original_stdin = dup(STDOUT_FILENO);
	/*
	MY CODE
	*/

	//const struct expr *e = line->head;
	//execute_command(&line->head->cmd);
	struct expr *e = line->head;
	int prev_fd[2];
	int fd[2];

	while (e != NULL) {
		if (e->type == EXPR_TYPE_COMMAND) {
			if (strcmp(e->cmd.exe, "cd") == 0){
				char currentDirectory[PATH_MAX];

				if (getcwd(currentDirectory, sizeof(currentDirectory)) == NULL) {
					perror("getcwd");
					//return 1;
				}

				char fullPath[PATH_MAX];
				snprintf(fullPath, sizeof(fullPath), "%s/%s", currentDirectory, e->cmd.args[0]);

				if (chdir(fullPath) != 0) {
					perror("chdir");
					//return 1;
				}

				//change_directory(e->cmd.args[0]);
				//if(change_directory(e->cmd.args[0])){
				//	printf("Change directory error!");
				//	exit(EXIT_FAILURE);
				//}
				//не уверен что прям в конец надо пересылать так как
				//если команда cd в центре pipe то кажется что пайп прервется
				goto after_command_line_execution;
			}

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
				// То что выводило в stdout будет выводить в fd[1]
				dup2(fd[1], STDOUT_FILENO);
				close(fd[1]);
				
				if(e != line->head){
					close(prev_fd[1]);
					dup2(prev_fd[0], STDIN_FILENO);
					close(prev_fd[0]);
				}
				
				//Потом тут нужно поменять команду, потому что
				//в распарсеных аргументах нет самой команды
				char *combined_args[e->cmd.arg_count + 2]; // +1 for the function name, +1 for the NULL
				combined_args[0] = e->cmd.exe;
				for (int i = 0; i < e->cmd.arg_count; i++) {
					combined_args[i + 1] = e->cmd.args[i];
				}
				combined_args[e->cmd.arg_count + 1] = NULL;
				
				if (strcmp(combined_args[0], "exit") == 0){
					//проверить на пустоту аргумента?
					//exit(atoi(combined_args[1]));
				} else {
					execvp(combined_args[0], combined_args);
					//execvp(e->cmd.exe, e->cmd.args);
					perror("execlp");
					exit(EXIT_FAILURE);
				}
			} else { // Parent process
				close(fd[1]);

				prev_fd[0] = fd[0];
				prev_fd[1] = fd[1];

				//wait(NULL);
				int status;
				waitpid(pid, &status, 0);
				if (WIFEXITED(status)) {
                	//printf("Child process exited with status %d\n", WEXITSTATUS(status));
            	}
			}
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
	
	
	bool outputChanged = false;
	//OUTPUT:
	if (line->out_type == OUTPUT_TYPE_STDOUT) {
		//printf("stdout\n");
	} else if (line->out_type == OUTPUT_TYPE_FILE_NEW) {
		//printf("new file - \"%s\"\n", line->out_file);
		int fd_out = open(line->out_file, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		if (fd_out == -1) {
			perror("open");
			exit(1);
		}
		dup2(fd_out, STDOUT_FILENO);
		close(fd_out);
		outputChanged = true;
	} else if (line->out_type == OUTPUT_TYPE_FILE_APPEND) {
		//printf("append file - \"%s\"\n", line->out_file);
		int fd_out = open(line->out_file, O_WRONLY | O_CREAT | O_APPEND, 0644);
		if (fd_out == -1) {
			perror("open");
			exit(1);
		}
		dup2(fd_out, STDOUT_FILENO);
		close(fd_out);
		outputChanged = true;
	} else {
		assert(false);
	}

	//Output
	char buffer[10000];
	ssize_t bytes_read = read(prev_fd[0], buffer, sizeof(buffer));
	close(prev_fd[0]);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0'; // Null-terminate the string
		printf("%s", buffer);
	} else {
		//printf("Child process did not produce any output.\n");
	}
	// dup2(original_stdout_end, STDOUT_FILENO);
    // close(original_stdout_end);
	if (outputChanged){
		if (dup2(original_stdout, STDOUT_FILENO) == -1) {
			perror("dup2 (reset)");
			exit(1);
		}
	}
	if (dup2(original_stdin, STDIN_FILENO) == -1) {
		perror("dup2 (reset)");
		exit(1);
	}
	after_command_line_execution:
	printf("");
	// Restore the original STDOUT descriptor (not reached if execvp is successful)
    

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
			for (int i = 0; i < e->cmd.arg_count; ++i)
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
int isFileDescriptorClosed(int fd) {
    return fcntl(fd, F_GETFD) == -1;
}


int main(){//_workingpipesusingwhile(){
	/////////////////////////
	//making a command line//
	/////////////////////////
	//echo 'source string' | sed 's/source/destination/g' | sed 's/string/value/g'
	//echo "source string" | sed "s/source/destination/g" | sed "s/string/value/g"
	struct command cmd_echo;
	cmd_echo.exe = "echo";
	char *args1[] = {"echo", "source string", NULL};
	cmd_echo.args = args1;
	cmd_echo.arg_count = 2;
	cmd_echo.arg_capacity = 2;

	struct command cmd_tail1;
	cmd_tail1.exe = "sed";
	char *args2[] = {"sed", "s/source/destination/g", NULL};
	cmd_tail1.args = args2;
	cmd_tail1.arg_count = 3;
	cmd_tail1.arg_capacity = 3;

	struct command cmd_tail2;
	cmd_tail2.exe = "sed";
	char *args3[] = {"sed", "s/string/value/g", NULL};
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
	struct command_line cmdline;
		
	cmdline.head = &expr_echo;
	cmdline.tail = &expr_tail2;
	cmdline.out_type = OUTPUT_TYPE_STDOUT;
	cmdline.out_file = NULL;
	cmdline.is_background = false;

	///////////////////////////
	//execution in while loop//
	///////////////////////////
	struct expr *e = cmdline.head;
	int prev_fd[2];
	int fd[2];

	while (e != NULL) {
		if (e->type == EXPR_TYPE_COMMAND) {

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
				// То что выводило в stdout будет выводить в fd[1]
				dup2(fd[1], STDOUT_FILENO);
				close(fd[1]);
				
				if(e != cmdline.head){
					close(prev_fd[1]);
					dup2(prev_fd[0], STDIN_FILENO);
					close(prev_fd[0]);
				}
				
				//Потом тут нужно поменять команду, потому что
				//в распарсеных аргументах нет самой команды
				execvp(e->cmd.exe, e->cmd.args);
				perror("execlp");
				exit(EXIT_FAILURE);
			} else { // Parent process
				close(fd[1]);

				prev_fd[0] = fd[0];
				prev_fd[1] = fd[1];
			}
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

	char buffer[10000];
	ssize_t bytes_read = read(prev_fd[0], buffer, sizeof(buffer));
	close(prev_fd[0]);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0'; // Null-terminate the string
		printf("Child process output:\n%s", buffer);
	} else {
		printf("Child process did not produce any output.");
	}

	return 0;
}





int main_goodpipeworkinginfor(){
	int prev_fd[2];
	int fd[2];
	for (int i = 0; i < 3; i++){

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
			// То что выводило в stdout будет выводить в fd[1]
			dup2(fd[1], STDOUT_FILENO);
			close(fd[1]);
			
			if(i != 0){
				close(prev_fd[1]);
				dup2(prev_fd[0], STDIN_FILENO);
				close(prev_fd[0]);
			}
			
			if (i == 0){
				execlp("ls","ls", "-l", NULL);
			} else if (i == 1) {
				execlp("grep","grep", "txt", NULL);
			} else if (i == 2) {
				execlp("awk", "awk", "{print $NF}", NULL);
			}
			perror("execlp");
			exit(EXIT_FAILURE);
		} else { // Parent process
			close(fd[1]);

			prev_fd[0] = fd[0];
			prev_fd[1] = fd[1];
			
			//printf("%d %d\n", fd[0], fd[1]);
		}
	}

	char buffer[10000];
	ssize_t bytes_read = read(prev_fd[0], buffer, sizeof(buffer));
	close(prev_fd[0]);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0'; // Null-terminate the string
		printf("Child process output:\n%s", buffer);
	} else {
		printf("Child process did not produce any output.");
	}

	//printf("File descriptors:\n");
	//printf("%d ", isFileDescriptorClosed(fd[0]));
	//printf("%d\n", isFileDescriptorClosed(fd[1]));
	//printf("%d ", isFileDescriptorClosed(prev_fd[0]));
	//printf("%d\n", isFileDescriptorClosed(prev_fd[1]));

	return 0;
}









int main_workingpipeswith3processes(){
	
	//////////////
	//PROCESS 1 //
	//////////////

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
		// То что выводило в stdout будет выводить в fd[1]
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

		execlp("ls","ls", "-l", NULL);
		perror("execlp");
		exit(EXIT_FAILURE);
	} else { // Parent process
		close(fd[1]);
	}

	//////////////
	//PROCESS 2 //
	//////////////

	int fd1[2];

	if (pipe(fd1) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid_t pid1 = fork();
	
	if (pid1 == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid1 == 0) {  // Child process
		close(fd1[0]);
		// То что выводило в stdout будет выводить в fd[1]
        dup2(fd1[1], STDOUT_FILENO);
        close(fd1[1]);

		dup2(fd[0], STDIN_FILENO);
		close(fd[0]);

		execlp("grep","grep", "txt", NULL);
		perror("execlp");
		exit(EXIT_FAILURE);
	} else { // Parent process
		close(fd1[1]);
	}

	//////////////
	//PROCESS 3 //
	//////////////

	//int fd2[2];

	if (pipe(fd) == -1) {
		perror("pipe");
		exit(EXIT_FAILURE);
	}

	pid_t pid2 = fork();
	
	if (pid2 == -1) {
		perror("fork");
		exit(EXIT_FAILURE);
	}

	if (pid2 == 0) {  // Child process
		close(fd[0]);
		// То что выводило в stdout будет выводить в fd[1]
        dup2(fd[1], STDOUT_FILENO);
        close(fd[1]);

		dup2(fd1[0], STDIN_FILENO);
		close(fd1[0]);

		execlp("awk", "awk", "{print $NF}", NULL);
		perror("execlp");
		exit(EXIT_FAILURE);
	} else { // Parent process
		close(fd[1]);
	}


	char buffer[10000];
	ssize_t bytes_read = read(fd[0], buffer, sizeof(buffer));
	close(fd[0]);

	if (bytes_read > 0) {
		buffer[bytes_read] = '\0'; // Null-terminate the string
		printf("Child process output:\n%s", buffer);
	} else {
		printf("Child process did not produce any output.");
	}
	
	return 0;
}

int main_previous() {
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

	
	//struct expr *e = cmdline.head;
	//struct expr *prev_e = NULL;
	
	

	// execlp("ls","ls", "-l", NULL);
	// perror("execlp");
	// exit(EXIT_FAILURE);
	int counter = 0;
	int prev_fd[2];

	for(int i = 0; i < 3; i++){
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
			if (prev_fd[0] != -1){
				close(prev_fd[1]); // Close the write end of the previous pipe
				dup2(prev_fd[0], STDIN_FILENO); // Redirect stdin to the read end of the previous pipe
				close(prev_fd[0]);
			}
			close(fd[0]); // Close the read end of the current pipe
			//Перенаправляем то что будет выводить этот процесс (в консоль) в пайп
			//Получается что STDOUT заменится на fd[1]
			dup2(fd[1], STDOUT_FILENO); 
			//И закроем прошлый дескриптор, так как он нам больше не нужен,
			//мы его уже скопировали в STDOUT_FILENO
			close(fd[1]);
			if (counter == 0){
				execlp("ls","ls", "-l", NULL);
			}else if (counter == 1) {
				execlp("grep", "grep", "txt", NULL);
			}else if (counter == 2){
				execlp("grep", "grep", "txt", NULL);
			}
			counter++;
			perror("execlp");
			exit(EXIT_FAILURE);
		} else {  // Parent process
			if (prev_fd[0] != -1) {
				close(prev_fd[0]);
				close(prev_fd[1]);
			}
			prev_fd[0] = fd[0];
			prev_fd[1] = fd[1];
		}
	}
	if (prev_fd[1] != -1) {
        close(prev_fd[1]);
    }
	 //Parent process should read the final output
    char buffer[10000];
    ssize_t bytes_read = read(prev_fd[0], buffer, sizeof(buffer));
    close(prev_fd[0]);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("Final output: %s", buffer);
    }
	/*
	while (e != NULL) {
		if (e->type == EXPR_TYPE_COMMAND) {
			if (e->next != NULL && e->next->type == EXPR_TYPE_PIPE){
				printf("Pipe will be after %s\n", e->cmd.exe);
				prev_e = e;
			} else if (e->next == NULL){
				printf("Last command %s\n", e->cmd.exe);
			}

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
				if (prev_fd[0] != -1){
					close(prev_fd[1]); // Close the write end of the previous pipe
					dup2(prev_fd[0], STDIN_FILENO); // Redirect stdin to the read end of the previous pipe
					close(prev_fd[0]);
				}
                close(fd[0]); // Close the read end of the current pipe
				//Перенаправляем то что будет выводить этот процесс (в консоль) в пайп
				//Получается что STDOUT заменится на fd[1]
                dup2(fd[1], STDOUT_FILENO); 
				//И закроем прошлый дескриптор, так как он нам больше не нужен,
				//мы его уже скопировали в STDOUT_FILENO
                close(fd[1]);
				if (counter == 0){
					execlp("ls","ls", "-l", NULL);
				}else if (counter == 1) {
					execlp("grep", "grep", "txt", NULL);
				}else if (counter == 2){
					execlp("grep", "grep", "txt", NULL);
				}
                counter++;
				perror("execlp");
				exit(EXIT_FAILURE);
            } else {  // Parent process
                if (prev_fd[0] != -1) {
                   close(prev_fd[0]);
                   close(prev_fd[1]);
                }
                prev_fd[0] = fd[0];
                prev_fd[1] = fd[1];
            }

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
	if (prev_fd[1] != -1) {
        close(prev_fd[1]);
    }
	 //Parent process should read the final output
    char buffer[10000];
    ssize_t bytes_read = read(prev_fd[0], buffer, sizeof(buffer));
    close(prev_fd[0]);

    if (bytes_read > 0) {
        buffer[bytes_read] = '\0'; // Null-terminate the string
        printf("Final output: %s", buffer);
    }
	*/
    // Wait for the first command to finish
    //int status;
    //waitpid(first_pid, &status, 0);

	printf("\nEND OF PROGRAM\n");

	// -rw-r--r--  1 arseniyrubtsov  staff     796 Sep 22 18:45 result15.txt
	// -rw-r--r--  1 arseniyrubtsov  staff     968 Sep 22 18:45 result20.txt
	// -rw-r--r--  1 arseniyrubtsov  staff    1078 Sep 22 18:45 result25.txt
	// -rw-r--r--  1 arseniyrubtsov  staff    5153 Sep 22 18:45 task_eng.txt
	// -rw-r--r--  1 arseniyrubtsov  staff    8487 Sep 22 18:45 task_rus.txt
	// -rw-r--r--  1 arseniyrubtsov  staff    2560 Sep 22 18:45 tests.txt
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
int main_test22(){
	struct command cmd2;
	cmd2.exe = "exit";
	char *args[] = {"exit", NULL};
	cmd2.args = args;
	cmd2.arg_count = 2;
	cmd2.arg_capacity = 2;
	execvp(cmd2.exe, cmd2.args);
	perror("execvp");
	return 0;
}

int
main_olddddd(void)
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
		//printf("%s", buf);
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
			//int original_stdout = dup(STDOUT_FILENO);
			execute_command_line(line);
			command_line_delete(line);
		}
		if (feof(stdin)) {
            break;
        }
	}
    
	parser_delete(p);
	return 0;
}
