#include"trx.h"

trxManager::trxManager(logManager* logMng):lm(new lockManager(this)),logMng(logMng){}
int trxManager::trx_begin(void){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	++trx_cnt;
	trxs.emplace(std::piecewise_construct,
				 std::forward_as_tuple(trx_cnt),
				 std::forward_as_tuple(trx_cnt, logMng));
	return trx_cnt;
}
int trxManager::trx_commit(int trx_id){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trxs[trx_id].commit(lm);
	trxs.erase(trx_id);
	return trx_id;
}
int trxManager::trx_abort(int trx_id, bufferManager* bm){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trxs[trx_id].abort(lm, bm);
	trxs.erase(trx_id);
	return trx_id;
}
int trxManager::trx_abort(trx_t& trx, bufferManager* bm){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trx.abort(lm, bm);
	int trx_id = trx.get_trx_id();
	trxs.erase(trx_id);
	return trx_id;
}

bool trxManager::dfs(std::unordered_map<int, bool>& visited, trx_t& trx, int start_id){
	bool flag = false;
	for(auto it = trx.begin(); it != trx.end();){
		auto now = trxs.find(*it);
		if(now == trxs.end()){
			trx.erase(it++);
			continue;
		}
		if(start_id == now->second.get_trx_id()) return true;
		bool* visit = &visited[now->second.get_trx_id()];
		if(*visit){
			it++;
			continue;
		}
		*visit = true;
		flag |= dfs(visited, now->second, start_id);
		it++;
	}
	return flag;
}

bool trxManager::is_dead_lock(trx_t& trx){
	auto visited = std::unordered_map<int, bool>();
	visited[trx.get_trx_id()] = true;
	return dfs(visited, trx, trx.get_trx_id());
}
int trxManager::record_lock(int table_id, int64_t key, int trx_id, bool is_write, lock_t* l){
	return lm->lock_acquire(table_id, key, trx_id, is_write ? EXCLUSIVE_LOCK : SHARED_LOCK, trx_manager_latch, l);
}

void trxManager::record_lock_wait(lock_t* l){
	return lm->lock_wait(l);
}

bool trxManager::find(int trx_id){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	return trxs.find(trx_id) != trxs.end();
}

int64_t trxManager::logging(int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	return trxs[trx_id].add_log(type, table_id, pageNum, offset, old_image, new_image);
}

trx_t& trxManager::operator [](int trx_id){
	return trxs[trx_id];
}
