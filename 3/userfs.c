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
	int block_list_count;
	int block_list_capacity;
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


void
free_file(struct file* file)
{
    struct block* block = file->block_list;
	struct block* free_block = NULL;
	
	//Free memory blocks
    while(block != NULL) {
        free_block = block;
		block = block->next;

        free(free_block->memory);
        free(free_block);
    }
	
	//Free name
    free(file->name);

	//Free file
    free(file);
}


int
ufs_open(const char *filename, int flags)
{
	//Checking if the file is already exists in file_list
	bool file_exists = false;
	struct file *current_file = file_list;

	while (current_file != NULL) {
		if (strcmp(current_file->name, filename) == 0){
			file_exists = true;
            break;
        }
        current_file = current_file->next;
    }

	//If file doesn't exist then create and add to file_list
	if (!file_exists && flags == UFS_CREATE){
		struct file * new_file = malloc(sizeof(struct file));
		//Allocating the memory for first block
		new_file->block_list = calloc(1, sizeof(struct block));
		new_file->block_list->memory = calloc(BLOCK_SIZE, sizeof(char*));
		new_file->block_list->occupied = 0;

		new_file->last_block = NULL;
		new_file->refs = 0;
		new_file->name = strdup(filename);
		new_file->next = NULL;
		new_file->prev = NULL;
		new_file->block_list_count = 0;
		new_file->block_list_capacity = 0;
		new_file->deleted = false;
		
		if (file_list == NULL){
			file_list = new_file;
		} else {
			new_file->next = file_list;
			file_list->prev = new_file;
			file_list = new_file;
		}

		current_file = new_file;
	} else {
		//Can't create and don't find
		if(!file_exists){
			ufs_error_code = UFS_ERR_NO_FILE;
			return -1;
		}
	}

	
	//If there is no place for new file_desc then allocate memory
	if (file_descriptor_count == file_descriptor_capacity){
		file_descriptor_capacity = (file_descriptor_capacity + 1) * 2;
		file_descriptors = realloc(file_descriptors, sizeof(*file_descriptors) * file_descriptor_capacity);

		//Intialize new descriptors to NULL
		for (int i = file_descriptor_count; i < file_descriptor_capacity; i++){
			file_descriptors[i] = NULL;
		}
	} else {
		assert(file_descriptor_count < file_descriptor_capacity);
	}
	
	//Find free file descriptor
	int fd;
	for (int i = 0; i < file_descriptor_capacity; i++){
		if (file_descriptors[i] == NULL){
			fd = i;
			break;
		}
	}
	
	//Assign file descriptor from free one
	file_descriptor_count++;
	current_file->refs++;

	file_descriptors[fd] = calloc(1, sizeof(struct filedesc));
	file_descriptors[fd]->file = current_file;
	// UFS_CREATE = 1
	// UFS_READ_ONLY = 2
	// UFS_WRITE_ONLY = 4
	// UFS_READ_WRITE = 8
	if (flags == UFS_READ_ONLY || flags == UFS_WRITE_ONLY){
		file_descriptors[fd]->flag = flags;
	} else {
		file_descriptors[fd]->flag = UFS_READ_WRITE;
	}
	file_descriptors[fd]->block_number = 0;
	file_descriptors[fd]->offset = 0;
	
	ufs_error_code = UFS_ERR_NO_ERR;
	return fd;
	// (void)filename;
	// (void)flags;
	// (void)file_list;
	// (void)file_descriptors;
	// (void)file_descriptor_count;
	// (void)file_descriptor_capacity;
	// ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	// return -1;
}

ssize_t
ufs_write(int fd, const char *buf, size_t size)
{
	//Checking for valid fd
	if (fd < 0 || fd >= file_descriptor_capacity || file_descriptors[fd] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
	
	//Checking for permission
    if (!(file_descriptors[fd]->flag == UFS_WRITE_ONLY) && !(file_descriptors[fd]->flag == UFS_READ_WRITE)){
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }

	struct block *count_block = file_descriptors[fd]->file->block_list;
	int file_occupied = 0;
	for (int i = 0; i < file_descriptors[fd]->block_number; i++){
		file_occupied += count_block->occupied;
	}

	//Checking for memory
	int new_file_size = file_occupied + size;
	
	if (MAX_FILE_SIZE < new_file_size){
		ufs_error_code = UFS_ERR_NO_MEM;
        return -1;
	}

	//Set the current block to write
	struct block *current_block = file_descriptors[fd]->file->block_list;
	for (int i = 0; i < file_descriptors[fd]->block_number; i++){
		current_block = current_block->next;
	}

	
	for (int i = 0; i < size; i++){
		if (file_descriptors[fd]->offset % BLOCK_SIZE == 0){
			file_descriptors[fd]->offset = 0;
			file_descriptors[fd]->block_number++;
			current_block->occupied = BLOCK_SIZE;

			if (current_block->next == NULL){
				current_block->next = calloc(1, sizeof(struct block));
				current_block->next->memory = calloc(BLOCK_SIZE, sizeof(char));
				current_block->next->occupied = 0;
			}
			current_block = current_block->next;
		}

		current_block->memory[file_descriptors[fd]->offset] = buf[i];
		
		file_descriptors[fd]->offset++;

		if (current_block->occupied < file_descriptors[fd]->offset) {
           	current_block->occupied++;
        }
	}


	ufs_error_code = UFS_ERR_NO_ERR;
	return size;
}

ssize_t
ufs_read(int fd, char *buf, size_t size)
{
	
	//Checking for valid fd
	if (fd < 0 || fd >= file_descriptor_capacity || file_descriptors[fd] == NULL) {
        ufs_error_code = UFS_ERR_NO_FILE;
        return -1;
    }
	
	//Checking for permission
    if (!(file_descriptors[fd]->flag == UFS_READ_ONLY) && !(file_descriptors[fd]->flag == UFS_READ_WRITE)){
        ufs_error_code = UFS_ERR_NO_PERMISSION;
        return -1;
    }
	
	//Set the current block to read
	struct block *current_block = file_descriptors[fd]->file->block_list;
	for (int i = 0; i < file_descriptors[fd]->block_number; i++){
		current_block = current_block->next;
	}
	
	int bytes_read = 0;
	for (int i = 0; i < size; i++){
		
		if (file_descriptors[fd]->offset == current_block->occupied && current_block->next == NULL){
			ufs_error_code = UFS_ERR_NO_ERR;
			return bytes_read;
		}
		
		if (file_descriptors[fd]->offset % BLOCK_SIZE == 0){
			file_descriptors[fd]->offset = 0;
			file_descriptors[fd]->block_number++;

			current_block = current_block->next;
		}

		buf[i] = current_block->memory[file_descriptors[fd]->offset];
        bytes_read++;
        file_descriptors[fd]->offset++;
	}
	
	
	ufs_error_code = UFS_ERR_NO_ERR;
	return bytes_read;
	// (void)fd;
	// (void)buf;
	// (void)size;
	// ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	// return -1;
}

int
ufs_close(int fd)
{	
	//If fd is in range and descriptor not NULL
	if(fd >= 0 && fd < file_descriptor_capacity && file_descriptors[fd] != NULL){
	    file_descriptors[fd]->file->refs--;

	    if (file_descriptors[fd]->file->refs == 0 && file_descriptors[fd]->file->deleted) {
	    	free_file(file_descriptors[fd]->file);
	    }

	    free(file_descriptors[fd]);
        file_descriptors[fd] = NULL;

		ufs_error_code = UFS_ERR_NO_ERR;	
        return 0;
	}
	
	ufs_error_code = UFS_ERR_NO_FILE;
	return -1;

	// (void)fd;
	// ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	// return -1;
}



int
ufs_delete(const char *filename)
{
	struct file *current_file = file_list;
	bool file_exists = false;

	while (current_file != NULL) {
		if (strcmp(current_file->name, filename) == 0){
			file_exists = true;
			break;
        }
        current_file = current_file->next;
    }

	if (file_exists){
		//Delete from list
		if (current_file->prev != NULL){
			current_file->prev->next = current_file->next;
		} else {
			file_list = current_file->next;
		}

		if (current_file->next != NULL) {
			current_file->next->prev = current_file->prev;
		}

		//Clear file
		if (current_file->refs == 0) {
			free_file(current_file);
		} else {
			current_file->deleted = true;
		}

		ufs_error_code = UFS_ERR_NO_ERR;
		return 0;
	} else {
		ufs_error_code = UFS_ERR_NO_FILE;
		return -1;
	}

	// (void)filename;
	// ufs_error_code = UFS_ERR_NOT_IMPLEMENTED;
	// return -1;
}

void
ufs_destroy(void)
{
    struct file* file_to_remove = file_list;
    while (file_to_remove != NULL) {
        free_file(file_to_remove);
        file_to_remove = file_to_remove->next;
    }
}
