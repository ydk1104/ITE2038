#include<lock.h>

lock_t* lockManager::lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode){
	return nullptr;
}
void lockManager::lock_release(lock_t* lock_obj){
}
