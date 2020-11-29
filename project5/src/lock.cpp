#include<lock.h>

lockManager::lockManager(trxManager* tm):tm(tm){}
lock_t* lockManager::lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode){
	std::unique_lock<std::mutex> lock(lock_manager_latch);
	return nullptr;
}
void lockManager::lock_release(lock_t* lock_obj){
	std::unique_lock<std::mutex> lock(lock_manager_latch);
}
