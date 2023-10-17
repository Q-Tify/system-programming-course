#include "parser.h"

#include <assert.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>

#include <stdio.h>

struct parser {
	char *buffer;
	uint32_t size;
	uint32_t capacity;
};

enum token_type {
	TOKEN_TYPE_NONE,
	TOKEN_TYPE_STR,
	TOKEN_TYPE_NEW_LINE,
	TOKEN_TYPE_PIPE,
	TOKEN_TYPE_AND,
	TOKEN_TYPE_OR,
	TOKEN_TYPE_OUT_NEW,
	TOKEN_TYPE_OUT_APPEND,
	TOKEN_TYPE_BACKGROUND,
};

struct token {
	enum token_type type;
	char *data;
	uint32_t size;
	uint32_t capacity;
};

static char *
token_strdup(const struct token *t)
{
	assert(t->type == TOKEN_TYPE_STR);
	assert(t->size > 0);
	char *res = malloc(t->size + 1);
	memcpy(res, t->data, t->size);
	// Null-terminate the result string to make it a valid C string
	res[t->size] = 0;
	return res;
}

//Добавляет букву в токен
static void
token_append(struct token *t, char c)
{
	if (t->size == t->capacity) {
		t->capacity = (t->capacity + 1) * 2;
		t->data = realloc(t->data, sizeof(*t->data) * t->capacity);
	} else {
		assert(t->size < t->capacity);
	}
	t->data[t->size++] = c;
}

//Makes token empty
// enum token_type type;
// char *data;
// int size;
// int capacity;
static void
token_reset(struct token *t)
{
	t->size = 0;
	t->type = TOKEN_TYPE_NONE;
}

static void
command_append_arg(struct command *cmd, char *arg)
{
	if (cmd->arg_count == cmd->arg_capacity) {
		cmd->arg_capacity = (cmd->arg_capacity + 1) * 2;
		cmd->args = realloc(cmd->args, sizeof(*cmd->args) * cmd->arg_capacity);
	} else {
		assert(cmd->arg_count < cmd->arg_capacity);
	}
	cmd->args[cmd->arg_count++] = arg;
}

void
command_line_delete(struct command_line *line)
{
	while (line->head != NULL) {
		struct expr *e = line->head;
		if (e->type == EXPR_TYPE_COMMAND) {
			struct command *cmd = &e->cmd;
			free(cmd->exe);
			for (uint32_t i = 0; i < cmd->arg_count; ++i)
				free(cmd->args[i]);
			free(cmd->args);
		}
		line->head = e->next;
		free(e);
	}
	free(line->out_file);
	free(line);
}

// struct command_line {
// 	struct expr *head;
// 	struct expr *tail;
// 	enum output_type out_type;
// 	/** Valid if the out type is FILE. */
// 	char *out_file;
// 	bool is_background;
// };

//iteration1
//head->e1
//tail->e1
//iteration2
//head->e1->e2
//tail->e2
//iteration3
//head->e1->e2->e3
//tail->e3
static void
command_line_append(struct command_line *line, struct expr *e)
{
	if (line->head == NULL)
		line->head = e;
	else
		line->tail->next = e;
	line->tail = e;
}


//Creating new parser 
//char *buffer;
//int size
//int capacity
struct parser *
parser_new(void)
{
	return calloc(1, sizeof(struct parser));
}

//Size
//This member of the struct parser represents the current size of the data stored in the buffer.
//Capacity 
//This member represents the maximum size that the parser's buffer can hold without resizing.
void
parser_feed(struct parser *p, const char *str, uint32_t len)
{
	// 0 - 0 first try
	uint32_t cap = p->capacity - p->size;
	// it will resize and increase its size
	if (cap < len) {
		uint32_t new_capacity = (p->capacity + 1) * 2;
		// if new capacity is not enough
		if (new_capacity - p->size < len)
			new_capacity = p->size + len;
		//sizeof(*p->buffer): This expression calculates the size of one element in the buffer. 
		p->buffer = realloc(p->buffer, sizeof(*p->buffer) * new_capacity);
		p->capacity = new_capacity;
	}
	//dest pointer, what to copy, how many
	memcpy(p->buffer + p->size, str, len);
	p->size += len;
	assert(p->size <= p->capacity);
}

static void
parser_consume(struct parser *p, uint32_t size)
{
	assert(p->size >= size);
	if (size == p->size) {
		p->size = 0;
		return;
	}
	memmove(p->buffer, p->buffer + size, p->size - size);
	p->size -= size;
}

static uint32_t
parse_token(const char *pos, const char *end, struct token *out)
{
	//empty the token for safety reasons
	token_reset(out);

	const char *begin = pos;
	
	//Если это пробел то пропускаем это, если это новая линия, 
	//то возвращаем токен новой линии и возвращаем pos + 1
	while (pos < end) {
		if (!isspace(*pos))
			break;
		if (*pos == '\n') {
			out->type = TOKEN_TYPE_NEW_LINE;
			return pos + 1 - begin;
		}
		++pos;
	}
	//Если прошли все пробелы и дошли до токена не встретив
	//перехода на новую строку начинаем парсить токен
	char quote = 0;
	while (pos < end) {
		char c = *pos;
		switch(c) {
		case '\'': //одинарная кавычка
		case '"': //двойная квычка
			//запоминаем тип кавычки по номеру
			if (quote == 0) {
				quote = c;
				++pos;
				if (pos == end)
					return 0;
				continue;
			}
			//Если разные кавычки встретили то идем в append and next
			//Получается что мы просто ждем еще одной кавычки пока
			//токен не закроется и мы его не вернем
			if (quote != c)
				goto append_and_next;

			//Если одинаковые то возвращаем "something"
			out->type = TOKEN_TYPE_STR;
			return pos + 1 - begin; //возвращаем used = pos + 1 - begin
		case '\\':
			//если \ внутри одинарных кавычек, то просто добавляем его в строку
			if (quote == '\'')
				goto append_and_next;
			//если \ внутри "\" то проверяем след символ если пустой, то ожидаем
			//так как он становится особым символом \n типа новой строки
			if (quote == '"') {
				++pos;
				//возвращаем 0 чтобы потом получить символ и продолжить со \ и тд
				if (pos == end)
					return 0;
				//если же есть символ дальше, то мы получаем этот символ
				c = *pos;
				switch (c)
				{
				//если это \\ то добавляем только последний "\"
				case '\\':
					goto append_and_next;
				//если это \n (новая строка) то тупо игнорируем это (хотя это тут как то не работает echo "helo \n ")
				//короче что-то странное тут не совсем понятно как работает
				case '\n':
					++pos;
					continue;
				case '"':
					goto append_and_next;
				default:
					break;
				}
				token_append(out, '\\');
				goto append_and_next;
			}
			assert(quote == 0);
			++pos;
			if (pos == end)
				return 0;
			c = *pos;
			if (c == '\n') {
				++pos;
				continue;
			}
			goto append_and_next;
		case '&':
		case '|':
		case '>':
			//my fix hope it works
			if (quote == '\'' || quote == '"')
				goto append_and_next;
			//end of my fix

			if (out->size > 0) {
				out->type = TOKEN_TYPE_STR;
				return pos - begin;
			}
			
			


			//check for || |> >> >| &|
			++pos;
			if (pos == end)
				return 0;
			
			//echo "ds" >> 
			//echo "ds" || 
			//echo "ds" &&
			if (*pos == c) {
				switch(c) {
				case '&':
					out->type = TOKEN_TYPE_AND;
					break;
				case '|':
					out->type = TOKEN_TYPE_OR;
					break;
				case '>':
					out->type = TOKEN_TYPE_OUT_APPEND;
					break;
				default:
					assert(false);
					break;
				}
				++pos;
			//echo "ds" & тут проверяем только 1 символ
			} else {
				switch(c) {
				case '&':
					out->type = TOKEN_TYPE_BACKGROUND;
					break;
				case '|':
					out->type = TOKEN_TYPE_PIPE;
					break;
				case '>':
					out->type = TOKEN_TYPE_OUT_NEW;
					break;
				default:
					assert(false);
					break;
				}
			}
			return pos - begin; //used
		case ' ':
		case '\t':
		case '\r':
			if (quote != 0)
				goto append_and_next;
			assert(out->size > 0);
			out->type = TOKEN_TYPE_STR;
			return pos + 1 - begin;
		case '\n':
			if (quote != 0)
				goto append_and_next;
			assert(out->size > 0);
			out->type = TOKEN_TYPE_STR;
			return pos - begin;
		case '#':
			if (quote != 0)
				goto append_and_next;
			if (out->size > 0) {
				out->type = TOKEN_TYPE_STR;
				return pos - begin;
			}
			//Пропускаем то что после # до начала новой строки
			++pos;
			while (pos < end) {
				if (*pos == '\n') {
					out->type = TOKEN_TYPE_NEW_LINE;
					return pos + 1 - begin;
				}
				++pos;
			}
			return 0;
		default:
			goto append_and_next;
		}
	append_and_next:
		//Добавляем букву в токен и идем дальше
		token_append(out, c);
		++pos;
	}
	return 0;
}

enum parser_error
parser_pop_next(struct parser *p, struct command_line **out)
{
	struct command_line *line = calloc(1, sizeof(*line));
	char *pos = p->buffer;
	const char *begin = pos;
	char *end = pos + p->size;

	struct token token = {0};
	enum parser_error res = PARSER_ERR_NONE;

	while (pos < end) {
		uint32_t used = parse_token(pos, end, &token);
		//used указывает там где мы закончили парсить прошлый токен
		if (used == 0)
			goto return_no_line;
		
		//Если получилось распарсить что-то
		pos += used;
		struct expr *e;
		// struct expr
		// enum expr_type type;
		/** Valid if the type is COMMAND. */
		// struct command cmd;
		// struct expr *next;

		// struct command {
		// 	char *exe;
		// 	char** args;
		// 	uint32_t arg_count;
		// 	uint32_t arg_capacity;

		switch(token.type) {
		case TOKEN_TYPE_STR:
			//Если добавляем этот токен не первый раз
			if (line->tail != NULL && line->tail->type == EXPR_TYPE_COMMAND) {
				//Добавляет аргумент
				command_append_arg(&line->tail->cmd, token_strdup(&token));
				continue;
			}
			//echo "df"
			//echo TOKEN_TYPE_STR
			//"df" TOKEN_TYPE_STR

			e = calloc(1, sizeof(*e));
			e->type = EXPR_TYPE_COMMAND;
			//конвертировали echo в правильную C строку
			e->cmd.exe = token_strdup(&token);
			command_line_append(line, e);
			continue;
		case TOKEN_TYPE_NEW_LINE:
			/* Skip new lines. */
			if (line->tail == NULL)
				continue;
			goto close_and_return;
		case TOKEN_TYPE_PIPE:
			if (line->tail == NULL) {
				res = PARSER_ERR_PIPE_WITH_NO_LEFT_ARG;
				goto return_error;
			}
			if (line->tail->type != EXPR_TYPE_COMMAND) {
				res = PARSER_ERR_PIPE_WITH_LEFT_ARG_NOT_A_COMMAND;
				goto return_error;
			}
			e = calloc(1, sizeof(*e));
			e->type = EXPR_TYPE_PIPE;
			command_line_append(line, e);
			continue;
		case TOKEN_TYPE_AND:
			if (line->tail == NULL) {
				res = PARSER_ERR_AND_WITH_NO_LEFT_ARG;
				goto return_error;
			}
			if (line->tail->type != EXPR_TYPE_COMMAND) {
				res = PARSER_ERR_AND_WITH_LEFT_ARG_NOT_A_COMMAND;
				goto return_error;
			}
			e = calloc(1, sizeof(*e));
			e->type = EXPR_TYPE_AND;
			command_line_append(line, e);
			continue;
		case TOKEN_TYPE_OR:
			if (line->tail == NULL) {
				res = PARSER_ERR_OR_WITH_NO_LEFT_ARG;
				goto return_error;
			}
			if (line->tail->type != EXPR_TYPE_COMMAND) {
				res = PARSER_ERR_OR_WITH_LEFT_ARG_NOT_A_COMMAND;
				goto return_error;
			}
			e = calloc(1, sizeof(*e));
			e->type = EXPR_TYPE_OR;
			command_line_append(line, e);
			continue;
		case TOKEN_TYPE_OUT_NEW:
		case TOKEN_TYPE_OUT_APPEND:
		case TOKEN_TYPE_BACKGROUND:
			goto close_and_return;
		default:
			assert(false);
		}
	}
	goto return_no_line;

close_and_return:
	if (token.type == TOKEN_TYPE_OUT_NEW || token.type == TOKEN_TYPE_OUT_APPEND)
	{
		if (token.type == TOKEN_TYPE_OUT_NEW)
			line->out_type = OUTPUT_TYPE_FILE_NEW;
		else
			line->out_type = OUTPUT_TYPE_FILE_APPEND;
		
		//Если после redirect >> или > пусто, то ничего не возвращаем
		uint32_t used = parse_token(pos, end, &token);
		if (used == 0)
			goto return_no_line;
		
		//Если что то есть, то мы это спарсили и проверяем чтобы это было строкой - файлом
		//куда мы записываем
		pos += used;
		if (token.type != TOKEN_TYPE_STR) {
			res = PARSER_ERR_OUTOUT_REDIRECT_BAD_ARG;
			goto return_error;
		}
		//Записываем в файл
		line->out_file = token_strdup(&token);

		//Если можно распарсить то проверяем
		used = parse_token(pos, end, &token);
		if (used == 0)
			goto return_no_line;
		pos += used;
	}

	if (token.type == TOKEN_TYPE_BACKGROUND) {
		line->is_background = true;
		//после того как встретили например & проверяем есть ли еще что-то
		uint32_t used = parse_token(pos, end, &token);
		if (used == 0)
			goto return_no_line;
		pos += used;
	}
	//После того как пользователь нажал enter
	if (token.type == TOKEN_TYPE_NEW_LINE) {
		assert(line->tail != NULL);
		parser_consume(p, pos - begin);
		if (line->tail->type != EXPR_TYPE_COMMAND) {
			res = PARSER_ERR_ENDS_NOT_WITH_A_COMMAND;
			goto return_no_line;
		}
		res = PARSER_ERR_NONE;
		*out = line;
		goto return_final;
	}
	res = PARSER_ERR_TOO_LATE_ARGUMENTS;
	goto return_error;

return_error:
	/*
	 * Try to skip the whole current line. It can't be executed but can't
	 * just crash here because of that.
	 */
	while (pos < end) {
		uint32_t used = parse_token(pos, end, &token);
		if (used == 0)
			break;
		pos += used;
		if (token.type == TOKEN_TYPE_NEW_LINE) {
			parser_consume(p, pos - begin);
			goto return_no_line;
		}
	}
	res = PARSER_ERR_NONE;
	goto return_no_line;

//Ничего не смогло распарсить
return_no_line:
	command_line_delete(line);
	*out = NULL;

return_final:
	free(token.data);
	return res;
}

void
parser_delete(struct parser *p)
{
	free(p->buffer);
	free(p);
}
