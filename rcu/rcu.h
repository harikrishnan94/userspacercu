//
// Created by hari on 13/9/17.
//

#ifndef RCU_H
#define RCU_H

struct rcu_thread_local_t;
struct rcu_global_t;
typedef struct rcu_thread_local_t rcu_thread_local_t;
typedef struct rcu_global_t rcu_global_t;

#define MAX_RCU_THREADS 256

extern int rcu_initialize(rcu_global_t *rcu_global, int max_threads);
extern int rcu_destroy(rcu_global_t *rcu_global);

extern rcu_thread_local_t *rcu_register_thread(rcu_global_t *rcu_global);
extern void rcu_unregister_thread(rcu_thread_local_t *rcu_local);

extern void rcu_read_lock(rcu_thread_local_t *rcu_local);
extern void rcu_read_unlock(rcu_thread_local_t *rcu_local);
extern void rcu_synchronize(rcu_thread_local_t *rcu_local);

#endif
