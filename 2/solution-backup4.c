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
#include <errno.h>
/*
Given errors:
echo "\
"

Assertion failed: (t->size > 0), function token_strdup, file parser.c, line 39.
*/


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


/* REPLACE THIS CODE WITH ACTUAL COMMAND EXECUTION */
// assert(line != NULL);
// printf("================================\n");
// printf("Command line:\n");
// printf("Is background: %d\n", (int)line->is_background);
// printf("Output: ");
// if (line->out_type == OUTPUT_TYPE_STDOUT) {
// 	printf("stdout\n");
// } else if (line->out_type == OUTPUT_TYPE_FILE_NEW) {
// 	printf("new file - \"%s\"\n", line->out_file);
// } else if (line->out_type == OUTPUT_TYPE_FILE_APPEND) {
// 	printf("append file - \"%s\"\n", line->out_file);
// } else {
// 	assert(false);
// }
// printf("Expressions:\n");


static void
execute_command_line(const struct command_line *line)
{
	printf("=======-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-=-==-=-=-=-=-=-=-=-=-==-=-==-=-=\n\n");
	int fd[2];
	int prev_fd[2];
	prev_fd[0] = -1;
	prev_fd[1] = -1;
	const struct expr *e = line->head;

	while (e != NULL) {
		if (e->type == EXPR_TYPE_COMMAND) {
			if (strcmp(e->cmd.exe, "cd") == 0){
				if(change_dir(e->cmd.args[0])){
					printf("Error occured while changing directory!\n");
					goto end_of_function;	
				}
			} else {
				if (pipe(fd) == -1) {
					perror("pipe");
					exit(EXIT_FAILURE);
				}

				pid_t pid = fork();
				
				if (pid == -1) {
					perror("fork");
					exit(EXIT_FAILURE);
				}

				if (pid == 0) {
					close(fd[0]);
					
					//Redirecting stdout
					dup2(fd[1], STDOUT_FILENO);
					close(fd[1]);

					//Redirecting stdin
					if(e != line->head){
						//Надо проверить но мне кажется что он уже закрыт хотя хз
						close(prev_fd[1]);
						dup2(prev_fd[0], STDIN_FILENO);
						close(prev_fd[0]);
					}
				
					char **combined_args = get_program_name_with_args(e->cmd);
					execvp(combined_args[0], combined_args);
					
					free(combined_args);
					exit(EXIT_SUCCESS);
				} else {
					close(fd[1]);

					prev_fd[0] = fd[0];
					prev_fd[1] = fd[1];
					

					// Wait for the child process to complete
					//waitpid(pid, &status, 0);
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
	
	//printf("%d", isFileDescriptorClosed(prev_fd[0]));
	//printf("%d", isFileDescriptorClosed(prev_fd[1]));
	int original_stdout = dup(STDOUT_FILENO);

	if (line->out_type == OUTPUT_TYPE_FILE_NEW) {
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
	}

	char buffer[1024];
	ssize_t bytes_read;
	while ((bytes_read = read(prev_fd[0], buffer, sizeof(buffer))) > 0) {
		buffer[bytes_read] = '\0';
		printf("%s", buffer);
	}
	close(prev_fd[0]);

	dup2(original_stdout, STDOUT_FILENO);
	close(original_stdout);

	//close(fd[0]);
	//close(fd[1]);
	//close(prev_fd[0]);
	//close(prev_fd[1]);
	//while (wait(NULL) != -1 || errno != ECHILD);

	end_of_function:
	printf("");
}


//EXIT надо добавить для одинарного и в общем может переработать

//Может обработать закрытие трубы и в ошибке заканчивать процесс если он не будет завершаться

int
main(void)
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
