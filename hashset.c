//
// Created by hari on 14/9/17.
//

#include "hashset.h"
#include <stdlib.h>
#include <math.h>

typedef struct hashbucket_t
{
	struct hashbucket_t *next;
	long val;
} hashbucket_t;

struct hashset_t
{
	uint64_t        element_count;
	hashbucket_t    **buckets;
	uint64_t        nbuckets;
};

#define BIG_CONSTANT(x) (x##LLU)

static uint64_t next_pow_2(uint64_t v)
{
	return (uint64_t) pow(2, ceil(log(v) / log(2)));
}

static uint64_t hash64(uint64_t k)
{
	k ^= k >> 33;
	k *= BIG_CONSTANT(0xff51afd7ed558ccd);
	k ^= k >> 33;
	k *= BIG_CONSTANT(0xc4ceb9fe1a85ec53);
	k ^= k >> 33;

	return k;
}

hashset_t *new_hashset(long initialCapacity)
{
	hashset_t *new_set = NULL;

	new_set = malloc(sizeof(hashset_t));
	new_set->nbuckets = next_pow_2(initialCapacity);
	new_set->buckets = malloc(sizeof(hashbucket_t *) * new_set->nbuckets);
	new_set->element_count = 0;

	return new_set;
}

void delete_hashset(hashset_t *set)
{
	free(set->buckets);
	free(set);
}

size_t hashset_size(hashset_t *set)
{
	return set->nbuckets * sizeof(hashbucket_t *) + set->element_count * sizeof(hashbucket_t) + sizeof(hashset_t);
}

bool hashset_insert(hashset_t *set, long val)
{
	hashbucket_t **bucket_head = set->buckets + (hash64(val) & (set->nbuckets - 1));

	for (hashbucket_t *bucket = *bucket_head; bucket != NULL; bucket = bucket->next)
	{
		if (bucket->val == val)
			return false;
	}

	hashbucket_t *new_bucket = malloc(sizeof(hashbucket_t));
	new_bucket->val = val;
	new_bucket->next = *bucket_head;
	*bucket_head = new_bucket;

	return true;
}

bool hashset_find(hashset_t *set, long val)
{
	hashbucket_t **bucket_head = set->buckets + (hash64(val) & (set->nbuckets - 1));

	for (hashbucket_t *bucket = *bucket_head; bucket != NULL; bucket = bucket->next)
	{
		if (bucket->val == val)
			return true;
	}

	return false;
}

bool hashset_remove(hashset_t *set, long val)
{
	hashbucket_t **bucket_head = set->buckets + (hash64(val) & (set->nbuckets - 1));
	hashbucket_t *bucket, *prev;

	for (bucket = *bucket_head, prev = NULL; bucket != NULL; prev = bucket, bucket = bucket->next)
	{
		if (bucket->val == val)
			break;
	}

	if (bucket == NULL)
		return false;

	if (prev)
		prev->next = bucket->next;
	else
		(*bucket_head) = bucket->next;

	return true;
}
