//---------------------------------------------------------------
// File: QueueMain.c
// Purpose: Main file with tests for a demonstration of a queue
//		as an array.
// Programming Language: C
// Author: Dr. Rick Coleman
// Date: February 11, 2002
//---------------------------------------------------------------
#define _GNU_SOURCE

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
#include <sys/types.h>
#include <sched.h>
#include "fifo.h"

typedef unsigned long long ticks;
#define NUM_THREADS 4
#define NUM_SAMPLES 2000000
#define NUM_CPUS 2

//static int numEnqueue = 0;
//static int numDequeue = 0;
static ticks dequeue_ticks = 0, enqueue_ticks = 0;
static int CUR_NUM_THREADS = NUM_THREADS;
struct queue_t *q;
pthread_barrier_t barrier;

struct init_info {
	uint32_t	cpu_id;
	pthread_barrier_t * barrier;
};

struct init_info info[NUM_CPUS];

#define INIT_INFO struct init_info

//An alternative way is to use rdtscp which will wait until all previous instructions have been executed before reading the counter; might be problematic on multi-core machines
static __inline__ ticks getticks_serial(void)
{
	ticks tsc;
	__asm__ __volatile__(
			"rdtscp;"
			"shl $32, %%rdx;"
			"or %%rdx, %%rax"
			: "=a"(tsc)
			  :
			  : "%rcx", "%rdx");

	return tsc;
}

//get number of ticks, could be problematic on modern CPUs with out of order execution
static __inline__ ticks getticks(void)
{
	ticks tsc;
	__asm__ __volatile__(
			"rdtsc;"
			"shl $32, %%rdx;"
			"or %%rdx, %%rax"
			: "=a"(tsc)
			  :
			  : "%rcx", "%rdx");

	return tsc;
}



void *worker_handler( void * in)
{
	//INIT_INFO * init = (INIT_INFO *) arg;
	//int cpu_id = init->cpu_id;
	//pthread_barrier_t *barrier = init->barrier;

	ticks start_tick,end_tick,diff_tick;

	int NUM_SAMPLES_PER_THREAD = NUM_SAMPLES/CUR_NUM_THREADS;
	ELEMENT_TYPE value;
	pthread_barrier_wait(&barrier);

	start_tick = getticks();
	for (int i=1;i<=NUM_SAMPLES_PER_THREAD;i++)
	{
		while(dequeue(q, &value) != 0);
		//printf("deq %d\n", (int)value);

	}
	end_tick = getticks();
	diff_tick = end_tick - start_tick;
	__sync_add_and_fetch(&dequeue_ticks,diff_tick);

	return 0;

}

void *enqueue_handler( void * in)
{
	//INIT_INFO * init = (INIT_INFO *) arg;
	//pthread_barrier_t *barrier = init->barrier;

	ticks start_tick,end_tick,diff_tick;

	int NUM_SAMPLES_PER_THREAD = NUM_SAMPLES/CUR_NUM_THREADS;
	pthread_barrier_wait(&barrier);

	start_tick = getticks();
	for (int i=1;i<=NUM_SAMPLES_PER_THREAD;i++)
	{	
		//printf("enq %d\n", i);
		while(enqueue(q, (ELEMENT_TYPE)i) != 0);

	}
	end_tick = getticks();
	diff_tick = end_tick - start_tick;
	__sync_add_and_fetch(&enqueue_ticks,diff_tick);

	return 0;

}


int main(int argc, char **argv)
{ 
	q = (struct queue_t*)malloc(sizeof(struct queue_t));
	queue_init(q);

	printf("Concurrent Enqueue Concurrent Dequeue\n");
	printf("NumSamples,EnqCycleCount,DeqCycleCount,NumThreads\n");
	for (int k=1;k<=NUM_THREADS;k=k*2)
		{
			CUR_NUM_THREADS = k;

			pthread_t *worker_threads;
			pthread_t *enqueue_threads;
#ifndef SINGLE_PRODUCER
			worker_threads = (pthread_t *) malloc(sizeof(pthread_t) * CUR_NUM_THREADS);
			enqueue_threads = (pthread_t *) malloc(sizeof(pthread_t) * CUR_NUM_THREADS);

			pthread_barrier_init(&barrier, NULL, CUR_NUM_THREADS * 2);

			for (int i = 0; i < CUR_NUM_THREADS; i++) {
				pthread_create(&enqueue_threads[i], NULL, enqueue_handler, NULL);
				pthread_create(&worker_threads[i], NULL, worker_handler, NULL);
			}

			for (int i = 0; i < CUR_NUM_THREADS; i++) {
				pthread_join(enqueue_threads[i], NULL);
				pthread_join(worker_threads[i], NULL);
			}
#else
			worker_threads = (pthread_t*) malloc(sizeof(pthread_t) * CUR_NUM_THREADS);

			pthread_barrier_init(&barrier, NULL, CUR_NUM_THREADS);

			for(int i=0; i < CUR_NUM_THREADS; i++) {
				info[i].cpu_id = i;
				info[i].barrier = &barrier;
				pthread_create(&worker_threads[i], NULL, worker_handler, info[i]);
			}

			for(int i=0; i<CUR_NUM_THREADS, i++) {
				pthread_join(worker_threads[i], NULL);
#endif

			printf("%d,%llu,%llu,%d\n", NUM_SAMPLES, enqueue_ticks/NUM_SAMPLES,  dequeue_ticks/NUM_SAMPLES, CUR_NUM_THREADS);
		}
	return 0;
}
