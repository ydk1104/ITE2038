#include"trx.h"

trxManager::trxManager():lm(new lockManager(this)){}
int trxManager::trx_begin(void){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trxs[++trx_cnt];
	return trx_cnt;
}
int trxManager::trx_commit(int trx_id){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trxs[trx_id].commit(lm);
	trxs.erase(trx_id);
	//global_min and trx_id comp
	return trx_id;
}
int trxManager::trx_abort(int trx_id){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trxs[trx_id].abort();
	return trx_id;
}
bool trxManager::isDeadLock(void){
	return false;
}
void trxManager::record_lock(int table_id, int64_t key, int trx_id, bool is_write){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	lock_t* lock_obj = lm->lock_acquire(table_id, key, trx_id, is_write ? EXCLUSIVE_LOCK : SHARED_LOCK);
}
