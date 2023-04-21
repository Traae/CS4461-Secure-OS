#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <err.h>
#include <time.h>
#include <pthread.h>
#include <unistd.h>

int
compare_func(const void *a, const void *b) {
	int ia = *((int*)a);
	int ib = *((int*)b);
	if (ia < ib) return -1;
	if (ia > ib) return 1;
	return 0;
}

struct sorter_args {
	int *a;
	size_t start;
	size_t finish;
};

void *
sorter(void *varg) {
	struct sorter_args *args = varg;

	// Sort the vector "v" starting at position "start" and
	// and going to "finish" - 1.

	qsort(args->a, args->finish - args->start,
			sizeof(*args->a), compare_func);
	return NULL;
}

void
do_the_sort(int *a, size_t sz, int nthreads) {
	
	// MultiThread Non-Recursive Merge-Style Sort (Let's call it a Braid Sort)

	// I originaly had a more complicated Idea, but I remebered the merge sort somehow managed to work faster, so I'll just do that.

	// create and array of sorter arguments,
	// fill arrays with each chunk
	// pass each chunk to its own thread, sort,
	// finally sort the chunks together in the parent thread.
	
	pthread_t thd[nthreads];
	struct sorter_args arg;
	struct sorter_args chunks[nthreads];
	
	
	printf("Sorting on %d threads\n", nthreads);

	size_t pivotSize = sz / nthreads;
	size_t begin = 0;
	size_t end = pivotSize;

	for(int i=0; i<nthreads; i++){
		chunks[i].a = a;
		chunks[i].start = begin;
		chunks[i].finish = end;
		begin = end + 1;
		if (i<nthreads-1) { end += pivotSize;}
		else { end = sz; }
	}

	for(int i=0; i<nthreads; i++){
		pthread_create(&thd[i], NULL, sorter, &chunks[i]);
	}

	for(int i=0; i<nthreads; i++){
		pthread_join(thd[i], NULL);
	}
	
	arg.a = a;
	arg.start = 0;
	arg.finish = sz;
	sorter(&arg);
	
}

bool
verifysorted(const int *a, size_t sz) {
	// Make sure the array really is sorted
	bool result = true;

	for (size_t i = 1; i < sz; i++) {
		if (a[i - 1] > a[i]){
			result = false;
			break;
		}
	}
	return result;
}

void printArray(const int *a, size_t sz) {
	for (size_t i = 0; i < sz; i++) {
		printf("%d, ", a[i]);
	}
	printf("\n");
}

double
convert_timespec(struct timespec *t) {
	double d = t->tv_nsec;
	d /= 1e9;
	d += t->tv_sec;
	return d;
}

int main()
{
	int *vec = NULL;
	size_t sz = 0;
	
	// Get the list
	printf("Type an integer and hit enter.\n Repeat til finished, then hit (control + d)\n");
	for (;;) {
		int *nextv, x;

		if (feof(stdin))
			break;
		scanf("%d", &x);

		nextv = realloc(vec, (sz + 1) * sizeof(*vec));
		if (nextv == NULL)
			err(1, "realloc");
		vec = nextv;
		vec[sz++] = x;
	}

	// Get the threads
	int nthreads;
	int cores = (int) sysconf(_SC_NPROCESSORS_ONLN);

	

	printf("\nYour list is length: %d.\n You may use any number of threads upto: %d,\n", sz, (sz-1));
	printf("But it is reccomended to use no more than the number of cores availible: %d\n", cores);
	printf("\nHow many threads would you like to use?\nAn invalid input will default to %d.\nN = ", cores);
	
	scanf("%d", &nthreads);
	if ( (nthreads < 1) || (nthreads >= sz) ) nthreads = cores;
	// print the array
	
	printf("Original Array:\n");
	printArray(vec, sz);

	// start the clock
	struct timespec start, finish;
	clock_gettime(CLOCK_MONOTONIC, &start);

	// do the sort
	do_the_sort(vec, sz, nthreads);

	// stop the clock
	clock_gettime(CLOCK_MONOTONIC, &finish);

	// print the array
	printf("Sorted Array:\n");
	printArray(vec, sz);

	// Verify the sort actually happened
	if (!verifysorted(vec, sz)) {
		printf("Your array isn't sorted. Fast and wrong is wrong.\n");
		return 1;
	}

	double t2 = convert_timespec(&finish), t1 = convert_timespec(&start);

	printf("Elapsed time: %f seconds\n", t2 - t1);
	return 0;
}

