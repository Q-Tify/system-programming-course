#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libcoro.h"
#include <time.h>
#include <limits.h>
/**
 * You can compile and run this code using the commands:
 *
 * $> gcc solution.c libcoro.c
 * $> ./a.out
 */


struct CoroutineTimer {
    long long totalCoroutineWorkTime;
	struct timespec coroutineStartTime, coroutineEndTime;
	long long timeQuantum;
};


static struct CoroutineTimer *
newCoroutineTimer(long long totalCoroutineWorkTime, long long timeQuantum){	
	struct CoroutineTimer *ct = malloc(sizeof(*ct));
	
	ct->totalCoroutineWorkTime = totalCoroutineWorkTime;
	ct->timeQuantum = timeQuantum;

	return ct;
};

static void
deleteCoroutineTimer(struct CoroutineTimer *ct)
{
	free(ct);
}



struct Node {
    char* data;
    struct Node* next;
};

struct Queue {
    struct Node* front;
    struct Node* rear;
};

struct Queue* 
createQueue() {
    struct Queue* queue = (struct Queue*)malloc(sizeof(struct Queue));
    if (!queue) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    queue->front = queue->rear = NULL;
    return queue;
}

int 
isEmpty(struct Queue* queue) {
    return (queue->front == NULL);
}

void 
enqueue(struct Queue* queue, const char* data) {
    struct Node* newNode = (struct Node*)malloc(sizeof(struct Node));
    if (!newNode) {
        perror("Memory allocation error");
        exit(EXIT_FAILURE);
    }
    newNode->data = strdup(data);
    newNode->next = NULL;

    if (isEmpty(queue)) {
        queue->front = queue->rear = newNode;
    } else {
        queue->rear->next = newNode;
        queue->rear = newNode;
    }
}

char* 
dequeue(struct Queue* queue) {
    if (isEmpty(queue)) {
        printf("Queue is empty.\n");
        return NULL;
    }

    struct Node* temp = queue->front;
    char* data = temp->data;
    queue->front = temp->next;

    free(temp);
    return data;
}

void 
displayQueue(struct Queue* queue) {
    struct Node* current = queue->front;
    while (current != NULL) {
        printf("%s ", current->data);
        current = current->next;
    }
    printf("\n");
}

void 
freeQueue(struct Queue* queue) {
    while (!isEmpty(queue)) {
        char* data = dequeue(queue);
        free(data);
    }
    free(queue);
}


long long 
timeSpentInMicroseconds(struct timespec startTime, struct timespec endTime){
	long long elapsed_ns = (endTime.tv_sec - startTime.tv_sec) * 1000000000LL +
                           (endTime.tv_nsec - startTime.tv_nsec);
    return elapsed_ns / 1000;
}


void 
swap(int* a, int* b)
{
    int t = *a;
    *a = *b;
    *b = t;
}


int 
partition(int arr[], int low, int high, struct CoroutineTimer* timer)
{
    int pivot = arr[high];
 
    int i = (low - 1);
 
    for (int j = low; j <= high - 1; j++) {
        if (arr[j] < pivot) {
            i++;
            swap(&arr[i], &arr[j]);
        }

		//Checking if the time quantum is over
		clock_gettime(CLOCK_MONOTONIC, &timer->coroutineEndTime);
		long long timeSpent = timeSpentInMicroseconds(timer->coroutineStartTime, timer->coroutineEndTime);
		if(timeSpent > timer->timeQuantum){
			timer->totalCoroutineWorkTime += timeSpent;
			coro_yield();
			clock_gettime(CLOCK_MONOTONIC, &timer->coroutineStartTime);
		}
		
    }

    swap(&arr[i + 1], &arr[high]);
    return (i + 1);
}
 

void 
quickSort(int arr[], int low, int high, struct CoroutineTimer* timer)
{
    if (low < high) {

        int pi = partition(arr, low, high, timer);

        quickSort(arr, low, pi - 1, timer);
        quickSort(arr, pi + 1, high, timer);
    }
}




struct my_context {
	char *name;
	struct Queue *queueOfFiles;
	int** arrayForSortedNumbersArrays;
	int* pCurrentNumbersArray;
	int* arrayForSizesOfArrays;
	int timeQuantum;
};


static struct my_context *
my_context_new(const char *name, struct Queue *queue, int** array1, int* p1, int* array2, long long timeQuantum){	

	//Init pointer to struct of context
	struct my_context *ctx = malloc(sizeof(*ctx));
	
	//Creating copy of a string name to make it independent
	ctx->name = strdup(name);

    ctx->queueOfFiles = queue;
	ctx->arrayForSortedNumbersArrays = array1;
	ctx->pCurrentNumbersArray = p1;
	ctx->arrayForSizesOfArrays = array2;
	ctx->timeQuantum = timeQuantum;

	//Returning pointer to context
	return ctx;
}

static void
my_context_delete(struct my_context *ctx)
{
	free(ctx);
}



/**
 * A function, called from inside of coroutines recursively. Just to demonstrate
 * the example. You can split your code into multiple functions, that usually
 * helps to keep the individual code blocks simple.
 */
/*
static void
other_function(const char *name, int depth)
{
	printf("%s: entered function, depth = %d\n", name, depth);
	coro_yield();
	if (depth < 3)
		other_function(name, depth + 1);
}
*/

/**
 * Coroutine body. This code is executed by all the coroutines. Here you
 * implement your solution, sort each individual file.
 */
static int
coroutine_func_f(void *context)
{
	struct my_context *ctx = context;
	char *name = ctx->name;

	//Creating the timer to track the time
	struct CoroutineTimer* timer = newCoroutineTimer(0, ctx->timeQuantum);
	clock_gettime(CLOCK_MONOTONIC, &timer->coroutineStartTime);

	//While we have files to sort
	while (!isEmpty(ctx->queueOfFiles)){

		char *fileName = dequeue(ctx->queueOfFiles);
		FILE *file = fopen(fileName, "r"); 

		if (file == NULL) {
			perror("Error opening the file");
			return 1;
		}
		
		int size = 256;
		int *arrayOfNumbers = (int *)malloc(size * sizeof(int));
		int currentNumberIndex = 0;
		int number;

		if (arrayOfNumbers == NULL) {
			perror("Memory allocation failed");
			fclose(file);
			return 1;
		}

		// Read numbers from the file and store them in the array
		while (fscanf(file, "%d", &number) == 1) {
			arrayOfNumbers[currentNumberIndex++] = number;

			if (currentNumberIndex == size) {
				size *= 2;
				int *newArrayOfNumbers = (int *)realloc(arrayOfNumbers, size * sizeof(int));

				if (newArrayOfNumbers == NULL) {
					perror("Memory reallocation failed");
					free(arrayOfNumbers);
					fclose(file);
					return 1;
				}

				arrayOfNumbers = newArrayOfNumbers;
			}
		}
		fclose(file);
		
		//Sorting
		quickSort(arrayOfNumbers, 0, currentNumberIndex - 1, timer);

		//Allocating the memory for array of sorted numbers
		ctx->arrayForSortedNumbersArrays[*ctx->pCurrentNumbersArray] = (int*)malloc(currentNumberIndex * sizeof(int));

		//Copying the arrayOfNumbers into arrayForSortedNumbersArrays
    	memcpy(ctx->arrayForSortedNumbersArrays[*ctx->pCurrentNumbersArray], arrayOfNumbers, currentNumberIndex * sizeof(int));

		//Remembering the size of the arrayOfNumbers
		ctx->arrayForSizesOfArrays[*ctx->pCurrentNumbersArray] = currentNumberIndex;

		*ctx->pCurrentNumbersArray += 1;

		//Freeing the allocated memory
		free(arrayOfNumbers);
		
		//printf("File processed: %s by %s\n", fileName, name);
		free(fileName);
	}

	
	clock_gettime(CLOCK_MONOTONIC, &timer->coroutineEndTime);
	timer->totalCoroutineWorkTime += timeSpentInMicroseconds(timer->coroutineStartTime, timer->coroutineEndTime);
	printf("Coroutine %s: \nWork time: %lld microseconds.\n", name, timer->totalCoroutineWorkTime);
	
	
	free(name);
	deleteCoroutineTimer(timer);
	my_context_delete(ctx);
	
	/* This will be returned from coro_status(). */
	return 0;
}



void mergeSortedArrays(int** arrays, int* arrayForSizesOfArrays, int numberOfArrays, int* mergedArray){
    
	//Creating an array to keep track of the current index for each array
    int* currentIndex = (int*)malloc(numberOfArrays * sizeof(int));
    for (int i = 0; i < numberOfArrays; i++) {
        currentIndex[i] = 0;
    }

    int mergedIndex = 0;
    while (1){
        int smallestValue = INT_MAX;
        int smallestArray = -1;

        //Find the smallest value among the current elements of all arrays
        for (int i = 0; i < numberOfArrays; i++) {
            if (currentIndex[i] < arrayForSizesOfArrays[i] && arrays[i][currentIndex[i]] < smallestValue) {
                smallestValue = arrays[i][currentIndex[i]];
                smallestArray = i;
            }
        }

        //If no smallest value is found, we are done
        if (smallestArray == -1) {
            break;
        }

        //Add the smallest value to the merged array
        mergedArray[mergedIndex++] = smallestValue;

        //Move the index in the smallest array forward
        currentIndex[smallestArray]++;
    }

    //Free the memory used for currentIndex
    free(currentIndex);
}




int
main(int argc, char **argv)
{
	//Creating variables for tracking the total work time of program
	struct timespec programStartTime, programEndTime;
	
	//Set the start time
	clock_gettime(CLOCK_MONOTONIC, &programStartTime);

	//Getting the number of coroutines (bonus task)
    int numberOfCoroutines = atoi(argv[2]);

	//Reading the target latency
	char *endptr;
    long long T = strtoll(argv[1], &endptr, 10);

    if (*endptr != '\0') {
        fprintf(stderr, "Invalid input: %s is not a valid number.\n", argv[1]);
        return 1;
    }
	
	//Calculating the time quantum
	long long timeQuantum = T / numberOfCoroutines;

	//Creating a queue from which coroutines will take file names
	struct Queue* queueOfFiles = createQueue();
    for (int i = 3; i < argc; i++) {
		enqueue(queueOfFiles, argv[i]);
    }
	
	//Creating a variable for storing the sorted arrays of numbers
    int** arrayForSortedNumbersArrays = (int**)calloc(argc - 3, sizeof(int*));
	
	//For tracking where it is possible to add sorted numbers from new file
	int currentNumbersArray = 0;
    int* pCurrentNumbersArray = &currentNumbersArray;
	
	//For tracking the size of each sorted numbers array
	int* arrayForSizesOfArrays = (int*)calloc(argc - 3, sizeof(int));


	//Coroutine schedular init
	coro_sched_init();

	//Create coroutines
	for (int i = 0; i < numberOfCoroutines; ++i) {
		char name[16];
		sprintf(name, "coro_%d", i);
		coro_new(coroutine_func_f, my_context_new(name, queueOfFiles, arrayForSortedNumbersArrays, pCurrentNumbersArray, arrayForSizesOfArrays, timeQuantum));
	}

	/* Wait for all the coroutines to end. */
	struct coro *c;
	while ((c = coro_sched_wait()) != NULL) {
		/*
		 * Each 'wait' returns a finished coroutine with which you can
		 * do anything you want. Like check its exit status, for
		 * example. Don't forget to free the coroutine afterwards.
		 */
		printf("Number of context switches: %lld\n", coro_switch_count(c));
		printf("Finished %d\n\n", coro_status(c));
		coro_delete(c);
	}

	//Finding the total number of numbers to define the merged array
	int mergedArraySize = 0; 
	for (int i = 0; i < argc - 3; i++) {
		mergedArraySize += arrayForSizesOfArrays[i];
	}
	
	//Allocating memory for mergedArray
	int* mergedArray = (int*)calloc(mergedArraySize, sizeof(int));

	//Merging all sorted arrays
	mergeSortedArrays(arrayForSortedNumbersArrays, arrayForSizesOfArrays, argc - 3, mergedArray);

	//Writing the mergedArray to a file
    FILE* file = fopen("merged_array.txt", "w");
    if (file == NULL) {
        perror("Error opening the file");
        return 1;
    }

    for (int i = 0; i < mergedArraySize; i++) {
        fprintf(file, "%d ", mergedArray[i]);
    }

    fclose(file);

	//Freeing the allocated memory
	free(arrayForSortedNumbersArrays);
	free(arrayForSizesOfArrays);
	freeQueue(queueOfFiles);
	free(mergedArray);
	

	//Set the end time
	clock_gettime(CLOCK_MONOTONIC, &programEndTime);

	long long elapsed_us = timeSpentInMicroseconds(programStartTime, programEndTime);
	printf("Total work time: %lld microseconds.\n", elapsed_us);

	return 0;
}
