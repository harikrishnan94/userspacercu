//
// Created by hari on 13/9/17.
//

#include "rcu.h"

#include <stdint.h>
#include <pthread.h>
#include <assert.h>
#include <memory.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <poll.h>

#define CACHE_LINE_SIZE 64

typedef _Atomic uint64_t counter_t;

/*
 *  NOTE:
 *      Local counter is incremented by 1, whereas global counter in incremented by 2.
 */
#define LOCAL_COUNTER_READ(rcu_local)   atomic_load(&(rcu_local)->rcu_local_counter)
#define LOCAL_COUNTER_SET(rcu_local, v) atomic_store(&(rcu_local)->rcu_local_counter, v)
#define GLOBAL_COUNTER_READ(rcu_global) atomic_load(&(rcu_global)->rcu_global_counter)
#define GLOBAL_COUNTER_INC(rcu_global)  atomic_store(&(rcu_global)->rcu_global_counter, (rcu_global)->rcu_global_counter + 2)

#define THREAD_IS_ONGOING(rcu_local) (LOCAL_COUNTER_READ(rcu_local) & 1)

struct rcu_thread_local_t
{
	counter_t rcu_local_counter;
	rcu_global_t *rcu_global;
	rcu_thread_local_t *next;
	bool free;
} __attribute__ ((aligned(CACHE_LINE_SIZE)));

struct rcu_global_t
{
	int max_threads;
	rcu_thread_local_t *rcu_local_inuselist;
	pthread_mutex_t thread_register_mutex;
	counter_t rcu_global_counter __attribute__ ((aligned(CACHE_LINE_SIZE)));
	rcu_thread_local_t rcu_local[];
} __attribute__ ((aligned(CACHE_LINE_SIZE)));

#define INITIALIZE_REGISTER_THREAD_MUTEX(rcu_global)	pthread_mutex_init(&(rcu_global)->thread_register_mutex, NULL)
#define DESTROY_REGISTER_THREAD_MUTEX(rcu_global)		pthread_mutex_destroy(&(rcu_global)->thread_register_mutex)

#define LOCK_REGISTER_THREAD(rcu_global)	pthread_mutex_lock(&(rcu_global)->thread_register_mutex)
#define UNLOCK_REGISTER_THREAD(rcu_global)	pthread_mutex_unlock(&(rcu_global)->thread_register_mutex)

int rcu_global_size(int max_threads)
{
	return max_threads * sizeof(rcu_thread_local_t) + sizeof(rcu_global_t);
}

int rcu_initialize(rcu_global_t *rcu_global, int max_threads)
{
	assert(rcu_global != NULL);
	assert(max_threads <= MAX_RCU_THREADS);

	int rc = INITIALIZE_REGISTER_THREAD_MUTEX(rcu_global);

	if (rc != 0)
		return rc;

	atomic_init(&rcu_global->rcu_global_counter, 0);
	rcu_global->rcu_local_inuselist = NULL;
	rcu_global->max_threads = max_threads;

	memset(rcu_global->rcu_local, 0, sizeof(rcu_thread_local_t) * max_threads);

	for (int i = 0; i < max_threads; i++)
	{
		atomic_init(&rcu_global->rcu_local[i].rcu_local_counter, 0);
		rcu_global->rcu_local[i].rcu_global = rcu_global;
		rcu_global->rcu_local[i].next = NULL;
		rcu_global->rcu_local[i].free = true;
	}

	return 0;
}

int rcu_destroy(rcu_global_t *rcu_global)
{
	assert(rcu_global != NULL);
	return DESTROY_REGISTER_THREAD_MUTEX(rcu_global);
}

rcu_thread_local_t *rcu_register_thread(rcu_global_t *rcu_global)
{
	rcu_thread_local_t *rcu_local = NULL;

	assert(rcu_global != NULL);
	assert(rcu_local != NULL);

	LOCK_REGISTER_THREAD(rcu_global);

	for (int i = 0, max_threads = rcu_global->max_threads; i < max_threads; i++)
	{
		if (rcu_global->rcu_local[i].free)
		{
			rcu_local = &rcu_global->rcu_local[i];
			rcu_local->free = false;
			rcu_local->next = rcu_global->rcu_local_inuselist;

			atomic_thread_fence(memory_order_seq_cst);

			rcu_global->rcu_local_inuselist = rcu_local;
			break;
		}
	}

	UNLOCK_REGISTER_THREAD(rcu_global);

	return rcu_local;
}

void rcu_unregister_thread(rcu_thread_local_t *rcu_local)
{
	rcu_global_t *rcu_global;
	rcu_thread_local_t *current, *prev;

	assert(rcu_local != NULL);
	assert(rcu_local->rcu_global != NULL);

	rcu_global = rcu_local->rcu_global;

	LOCK_REGISTER_THREAD(rcu_global);

	rcu_local->free = true;

	for (prev = NULL, current = rcu_global->rcu_local_inuselist; current != NULL; current = current->next)
	{
		if (current == rcu_local)
		{
			if (prev)
				prev->next = current->next;
			else
				rcu_global->rcu_local_inuselist = current->next;

			break;
		}

		prev = current;
	}

	UNLOCK_REGISTER_THREAD(rcu_global);
}

void rcu_read_lock(rcu_thread_local_t *rcu_local)
{
	assert(rcu_local != NULL);
	assert(rcu_local->rcu_global != NULL);

	LOCAL_COUNTER_SET(rcu_local, GLOBAL_COUNTER_READ(rcu_local->rcu_global) + 1);
}

void rcu_read_unlock(rcu_thread_local_t *rcu_local)
{
	assert(rcu_local != NULL);
	assert(rcu_local->rcu_global != NULL);

	LOCAL_COUNTER_SET(rcu_local, GLOBAL_COUNTER_READ(rcu_local->rcu_global));
}

void rcu_synchronize(rcu_thread_local_t *rcu_local)
{
	assert(rcu_local != NULL);
	assert(rcu_local->rcu_global != NULL);

	rcu_global_t *rcu_global = rcu_local->rcu_global;
	counter_t global_counter;

	GLOBAL_COUNTER_INC(rcu_global);
	global_counter = GLOBAL_COUNTER_READ(rcu_global);

	for (rcu_thread_local_t *current = rcu_global->rcu_local_inuselist; current != NULL; current = current->next)
	{
		if (THREAD_IS_ONGOING(current) && global_counter < LOCAL_COUNTER_READ(current))
			poll(NULL, 0, 1);
	}
}
