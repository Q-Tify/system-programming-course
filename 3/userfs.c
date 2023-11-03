#include "userfs.h"
#include <stddef.h>

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

enum {
	BLOCK_SIZE = 512,
	MAX_FILE_SIZE = 1024 * 1024 * 100,
};

/** Global error code. Set from any function on any error. */
static enum ufs_error_code ufs_error_code = UFS_ERR_NO_ERR;

struct block {
	/** Block memory. */
	char *memory;
	/** How many bytes are occupied. */
	int occupied;
	/** Next block in the file. */
	struct block *next;
	/** Previous block in the file. */
	struct block *prev;

	/* PUT HERE OTHER MEMBERS */
};

struct file {
	/** Double-linked list of file blocks. */
	struct block *block_list;
	/**
	 * Last block in the list above for fast access to the end
	 * of file.
	 */
	struct block *last_block;
	/** How many file descriptors are opened on the file. */
	int refs;
	/** File name. */
	char *name;
	/** Files are stored in a double-linked list. */
	struct file *next;
	struct file *prev;

	/* PUT HERE OTHER MEMBERS */
	bool deleted;
};

/** List of all files. */
static struct file *file_list = NULL;

struct filedesc {
	struct file *file;

	/* PUT HERE OTHER MEMBERS */
	int flag;
	int block_number;
	int offset;
};

/**
 * An array of file descriptors. When a file descriptor is
 * created, its pointer drops here. When a file descriptor is
 * closed, its place in this array is set to NULL and can be
 * taken by next ufs_open() call.
 */
static struct filedesc **file_descriptors = NULL;
static int file_descriptor_count = 0;
static int file_descriptor_capacity = 0;

enum ufs_error_code
ufs_errno()
{
	return ufs_error_code;
}


/**
 * Find a file by filename in file_list.
 * @param filename Name of a file to find.
 *
 * @retval A pointer to the found file (struct file *) if found, or NULL if not found.
 */
static struct file*
find_file_in_file_list(const char *filename){
	struct file *current_file = file_list;
	struct file *found_file = NULL;

	while (current_file != NULL) {
		if (strcmp(current_file->name, filename) == 0){
			found_file = current_file;
            break;
        }
        current_file = current_file->next;
    }

	return found_file;
}


/**
 * Creates an empty block.
 * @retval A pointer to the created block (struct block *).
 */
static struct block*
create_new_block(){
	struct block* new_block = calloc(1, sizeof(struct block));
	new_block->memory = calloc(BLOCK_SIZE, sizeof(char*));
	new_block->occupied = 0;
	new_block->next = NULL;
	new_block->prev = NULL;
	return new_block;
}

/**
 * Creates an empty file with specified filename.
 * @param filename Name of a file to create.
 *
 * @retval A pointer to the created file (struct file *).
 */
static struct file *
create_new_file(const char *filename){
	struct file * new_file = malloc(sizeof(struct file));
	
	new_file->block_list = create_new_block();
	
	new_file->last_block = NULL;
	new_file->name = strdup(filename);
	new_file->refs = 0;
	new_file->deleted = false;

	new_file->next = NULL;
	new_file->prev = NULL;
	
	return new_file;
}


/**
 * Add new file to file_list.
 * @param file Pointer to (struct file*) of a file to add.
 */
static void
add_file_to_file_list(struct file *file){
	if (file_list == NULL){
		file_list = file;
	} else {
		file->next = file_list;
		file_list->prev = file;
		file_list = file;
	}
}


/**
 * Removes file from file_list.
 * @param file Pointer to (struct file*) of a file to remove.
 */
static void
remove_file_from_file_list(struct file *file){
	if (file->prev != NULL){
		file->prev->next = file->next;
	} else {
		file_list = file->next;
	}

	if (file->next != NULL) {
		file->next->prev = file->prev;
	}
}


/**
 * Resize file_descriptors if file_descriptor_count == file_descriptor_capacity.
 */
static void
resize_file_descriptors_if_full(){
	if (file_descriptor_count == file_descriptor_capacity){
		file_descriptor_capacity = (file_descriptor_capacity + 1) * 2;
		file_descriptors = realloc(file_descriptors, sizeof(*file_descriptors) * file_descriptor_capacity);

		//Intialize new descriptors to NULL
		for (int i = file_descriptor_count; i < file_descriptor_capacity; i++){
			file_descriptors[i] = NULL;
		}
	}
	assert(file_descriptor_count < file_descriptor_capacity);
}

/**
 * Returns free file descriptor.
 * @retval > 0 File descriptor.
 * @retval -1. Error occured.
 */
static int
get_free_file_descriptor(){
	int fd = -1;
	for (int i = 0; i < file_descriptor_capacity; i++){
		if (file_descriptors[i] == NULL){
			fd = i;
			break;
		}
	}
	return fd;	
}


int
ufs_open(const char *filename, int flags)
{
	struct file *found_file = find_file_in_file_list(filename);

	struct file *file = NULL;

	if (found_file == NULL){
		if (flags == UFS_CREATE){
			file = create_new_file(filename);
			add_file_to_file_list(file);
		} else {
			ufs_error_code = UFS_ERR_NO_FILE;
			return -1;
		}
	} else {
		file = found_file;
	}

	resize_file_descriptors_if_full();

	int fd = get_free_file_descriptor();

	if (fd == -1){
		printf("No free file descriptor, probably allocation of memory in resize_file_descriptors_if_full() has failed or you forgot to increase file_descriptor_count.");
		ufs_error_code = UFS_ERR_NO_MEM;
		return -1;
	} else {
		file_descriptor_count++;
	}

	file_descriptors[fd] = calloc(1, sizeof(struct filedesc));
	
	file_descriptors[fd]->file = file;
	file->refs++;
	
	if (flags == UFS_READ_ONLY || flags == UFS_WRITE_ONLY){
		file_descriptors[fd]->flag = flags;
	} else {
		file_descriptors[fd]->flag = UFS_READ_WRITE;
	}

	file_descriptors[fd]->block_number = 0;
	file_descriptors[fd]->offset = 0;

	ufs_error_code = UFS_ERR_NO_ERR;
	return fd;
}


/**
 * Checks that file descriptor is NULL.
 * @param fd File descriptor.
 * @retval true File descriptor is NULL.
 * @retval false File descriptor is NOT NULL.
 */
static bool
is_null(int fd){
	return file_descriptors[fd] == NULL;
}


/**
 * Checks that file descriptor is between 0 and file_descriptor_capacity.
 * @param fd File descriptor.
 * @retval true File descriptor is 0 <= fd < file_descriptor_capacity.
 * @retval false Not in range.
 */
static bool
is_in_range(int fd){
	return (fd >= 0 && fd < file_descriptor_capacity);
}


/**
 * Checks that file descriptor is in range and not NULL.
 * @param fd File descriptor.
 * @retval 0 File descriptor is valid.
 * @retval -1 File descriptor is not valid.
 */
static int
is_valid_file_descriptor(int fd){
	if (!is_in_range(fd) || is_null(fd)) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
	return 0;
}


/**
 * Checks that file descriptor is in range and not NULL.
 * @param fd File descriptor.
 * @param flag Required permission.
 * @retval 0 Has required permission.
 * @retval -1 No permission.
 */
static int
has_permission(int fd, int flag){
	if (!(file_descriptors[fd]->flag == flag) && !(file_descriptors[fd]->flag == UFS_READ_WRITE)){
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }
	return 0;
}


/**
 * Returns the current file size in bytes.
 * @param file Pointer (struct file *) to the file.
 * @retval >= 0 File size in bytes.
 */
static int
get_file_size(struct file* file){
	int file_size = 0;
	struct block *block = file->block_list;
	while (block != NULL){
		file_size += block->occupied;
		block = block->next;
	}
	return file_size;
}


/**
 * Checks that it is enough memory to write.
 * @param file Pointer (struct file *) to the file.
 * @param size How many bytes we want to write to the file.
 * @retval 0 Has enough memory to write.
 * @retval -1 Not enough memory.
 */
static int
has_memory(struct file* file, int size){
	int file_size = get_file_size(file);
	if (MAX_FILE_SIZE < file_size + size){
		ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
	}
	return 0;
}


/**
 * Returns the pointer to the block of file to which filedesc is pointing.
 * @param fd File descriptor.
 * @retval Pointer (struct block *) to the block of file.
 */
static struct block*
get_file_descriptor_block(int fd){
	struct block *block = file_descriptors[fd]->file->block_list;
	for (int i = 0; i < file_descriptors[fd]->block_number; i++){
		block = block->next;
	}
	return block;
}


ssize_t
ufs_write(int fd, const char *buf, size_t size)
{	
	int result = is_valid_file_descriptor(fd);
	if (result != 0) return result;

	result = has_permission(fd, UFS_WRITE_ONLY);
	if (result != 0) return result;

	result = has_memory(file_descriptors[fd]->file, size);
	if (result != 0) return result;

	struct block *write_block = get_file_descriptor_block(fd);

	for (size_t i = 0; i < size; i++){
		write_block->memory[file_descriptors[fd]->offset] = buf[i];
		file_descriptors[fd]->offset++;

		//If another fd overwrites the bytes, don't increase the size of block
		if (write_block->occupied < file_descriptors[fd]->offset){
           	write_block->occupied++;
        }

		if (file_descriptors[fd]->offset % BLOCK_SIZE == 0){
			file_descriptors[fd]->block_number++;
			file_descriptors[fd]->offset = 0;

			if (write_block->next == NULL){
				write_block->next = create_new_block();
			}

			write_block = write_block->next;
		}
	}
	
	ufs_error_code = UFS_ERR_NO_ERR;
	return size;
}


ssize_t
ufs_read(int fd, char *buf, size_t size)
{
	int result = is_valid_file_descriptor(fd);
	if (result != 0) return result;

	result = has_permission(fd, UFS_READ_ONLY);
	if (result != 0) return result;

	struct block * read_block = get_file_descriptor_block(fd);

	int bytes_read = 0;

	for (size_t i = 0; i < size; i++){
		//Check for EOF
		if (file_descriptors[fd]->offset == read_block->occupied && read_block->next == NULL){
			ufs_error_code = UFS_ERR_NO_ERR;
			return bytes_read;
		}

		buf[i] = read_block->memory[file_descriptors[fd]->offset];
        bytes_read++;
        file_descriptors[fd]->offset++;

		if (file_descriptors[fd]->offset % BLOCK_SIZE == 0){
			file_descriptors[fd]->block_number++;
			file_descriptors[fd]->offset = 0;

			read_block = read_block->next;
		}
	}

	ufs_error_code = UFS_ERR_NO_ERR;
	return bytes_read;
}


static void
free_file(struct file* file)
{
    struct block* block = file->block_list;
	struct block* free_block = NULL;
	
    while(block != NULL) {
        free_block = block;
		block = block->next;

        free(free_block->memory);
        free(free_block);
    }
	
    free(file->name);
    free(file);
}


int
ufs_close(int fd)
{	
	if(!is_in_range(fd) || is_null(fd)){
		ufs_error_code = UFS_ERR_NO_FILE;
		return -1;
	}

	file_descriptors[fd]->file->refs--;

	if (file_descriptors[fd]->file->refs == 0 && file_descriptors[fd]->file->deleted) {
		free_file(file_descriptors[fd]->file);
	}

	free(file_descriptors[fd]);
	file_descriptors[fd] = NULL;
	file_descriptor_count--;

	ufs_error_code = UFS_ERR_NO_ERR;	
	return 0;
}


int
ufs_delete(const char *filename)
{
	struct file *found_file = find_file_in_file_list(filename);

	if (found_file != NULL){
		remove_file_from_file_list(found_file);

		if (found_file->refs == 0) {
			free_file(found_file);
		} else {
			found_file->deleted = true;
		}

		ufs_error_code = UFS_ERR_NO_ERR;
		return 0;
	} else {
		ufs_error_code = UFS_ERR_NO_FILE;
		return -1;
	}
}


int
ufs_resize(int fd, size_t new_size){
	int result = is_valid_file_descriptor(fd);
	if (result != 0) return result;

	result = has_permission(fd, UFS_WRITE_ONLY);
	if (result != 0) return result;
	
	int file_size = get_file_size(file_descriptors[fd]->file);
	result = has_memory(file_descriptors[fd]->file, new_size - file_size);
	if (result != 0) return result;

	if (file_size == (int)new_size){
		ufs_error_code = UFS_ERR_NO_ERR;
        return 0;
	}

	if (file_size < (int)new_size){
		int block_number = file_descriptors[fd]->block_number;
        int offset = file_descriptors[fd]->offset;
		int diff = new_size - file_size;

        char* buf = calloc(diff, sizeof(char));
        int res = ufs_write(fd, buf, diff);

        file_descriptors[fd]->block_number = block_number;
        file_descriptors[fd]->offset = offset;

        if (res < 0) {
			return res;
		}
	} else {
		int new_block_number = new_size / BLOCK_SIZE;
		int new_offset = new_size % BLOCK_SIZE;

		struct block *block = get_file_descriptor_block(fd);
		struct block* delete_block = block->next;
		block->occupied = new_offset;
		block->next = NULL;
		
		//Free memory blocks
		struct block* free_block = NULL;
		while(delete_block != NULL) {
			free_block = delete_block;
			delete_block = delete_block->next;

			free(free_block->memory);
			free(free_block);
		}

		for (int i = 0; i < file_descriptor_capacity; i++){
			if (file_descriptors[i] != NULL){
				if (file_descriptors[i]->file == file_descriptors[fd]->file){
					if (file_descriptors[i]->block_number == new_block_number){
						if (file_descriptors[i]->offset > new_offset){
							file_descriptors[i]->offset = new_offset;
							block = get_file_descriptor_block(i);
							block->occupied = new_offset;
						}
					} else if (file_descriptors[i]->block_number > new_block_number) {
						file_descriptors[i]->block_number = new_block_number;
						file_descriptors[i]->offset = new_offset;
						block = get_file_descriptor_block(i);
						block->occupied = new_offset;
					}
				}
			}
		}
	}

	ufs_error_code = UFS_ERR_NO_ERR;
	return 0;
}


void
ufs_destroy(void)
{
	struct file* file_to_remove = file_list;
    while (file_to_remove != NULL) {
        free_file(file_to_remove);
        file_to_remove = file_to_remove->next;
    }
	free(file_descriptors);
}
