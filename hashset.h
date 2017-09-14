//
// Created by hari on 14/9/17.
//

#ifndef RCU_HASHSET_H
#define RCU_HASHSET_H

#include <stddef.h>
#include <stdbool.h>
#include <stdint.h>

struct hashset_t;
typedef struct hashset_t hashset_t;

extern hashset_t *new_hashset(long initialCapacity);
extern void delete_hashset(hashset_t *set);
extern size_t hashset_size(hashset_t *set);

extern bool hashset_insert(hashset_t *set, long val);
extern bool hashset_find(hashset_t *set, long val);
extern bool hashset_remove(hashset_t *set, long val);

#endif //RCU_HASHSET_H
