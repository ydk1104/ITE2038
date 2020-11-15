#ifndef __LOCK_TABLE_H__
#define __LOCK_TABLE_H__

#include <condition_variable>
#include <mutex>
#include <stdint.h>
#include <unordered_map>

typedef struct lock_t lock_t;

/* APIs for lock table */
int init_lock_table();
lock_t* lock_acquire(int table_id, int64_t key);
int lock_release(lock_t* lock_obj);

#endif /* __LOCK_TABLE_H__ */
