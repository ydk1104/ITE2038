#include"trx.h"

trxManager::trxManager():lm(new lockManager(this)){}
int trxManager::trx_begin(void){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	++trx_cnt;
	trxs.emplace(trx_cnt, trx_cnt);
	return trx_cnt;
}
int trxManager::trx_commit(int trx_id){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trxs[trx_id].commit(lm);
	trxs.erase(trx_id);
	return trx_id;
}
int trxManager::trx_abort(int trx_id){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trxs[trx_id].abort(lm);
	trxs.erase(trx_id);
	return 1;
//	return trx_id;
}
int trxManager::trx_abort(trx_t& trx){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trx.abort(lm);
	int trx_id = trx.get_trx_id();
	trxs.erase(trx_id);
	return 1;
//	return trx_id;
}

bool trxManager::dfs(std::unordered_map<int, bool>& visited, trx_t& trx){
	bool flag = false;
	for(auto i : visited) printf("%d %d\n", i.first, i.second);
	for(auto it = trx.begin(); it != trx.end();){
		auto now = trxs.find(*it);
		if(now == trxs.end()){
			trx.erase(it++);
			continue;
		}
		bool* visit = &visited[now->second.get_trx_id()];
		if(*visit) return true;
		*visit = true;
		flag |= dfs(visited, now->second);
		it++;
	}
	return flag;
}

bool trxManager::is_dead_lock(trx_t& trx){
	auto visited = std::unordered_map<int, bool>();
	visited[trx.get_trx_id()] = true;
	return dfs(visited, trx);
}
bool trxManager::record_lock(int table_id, int64_t key, int trx_id, bool is_write){
	return lm->lock_acquire(table_id, key, trx_id, is_write ? EXCLUSIVE_LOCK : SHARED_LOCK, trx_manager_latch);
}

bool trxManager::find(int trx_id){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	return trxs.find(trx_id) != trxs.end();
}

void trxManager::logging(int type, int table_id, int64_t key, char* value, int trx_id){
	std::unique_lock<std::mutex> lock(trx_manager_latch);
	trxs[trx_id].add_log(type, table_id, key, value);
}

trx_t& trxManager::operator [](int trx_id){
	return trxs[trx_id];
}
