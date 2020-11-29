#ifndef __TRX__
#define __TRX__

#include"lock.h"
#include<list>
#include<mutex>
#include<unordered_map>
#include<vector>

class trx_t{
private:
	const int trx_id;
	std::list<int> edge;
	std::vector<lock_t*> lock;
	int log[1];
	int acquired_lock = -1;
// log;

public:
	trx_t():trx_t(0){}
	trx_t(int trx_id):trx_id(trx_id){}
	int lock_mode;
	const int get_trx_id()const{return trx_id;}
	void end(lockManager* lm){
		for(auto i:lock) lm->lock_release(i);
	}
	void commit(lockManager* lm){
		end(lm);
	}
	void abort(lockManager* lm){
		for(auto i:log);
		end(lm);
	}
	void add_edge(int x){
		if(x==trx_id) return; // self loop is an-available;
		edge.push_back(x);
	}
	std::list<int>::iterator begin(){
		return edge.begin();
	}
	std::list<int>::iterator end(){
		return edge.end();
	}
	void erase(std::list<int>::iterator it){
		edge.erase(it);
	}
	void add_lock(lock_t* lock_obj){
		lock.push_back(lock_obj);
	}
	bool lock_acquired(int lock_mode){
		if(acquired_lock < lock_mode){
			acquired_lock = lock_mode;
			return false;
		}
		return true;
	}
};

class trxManager{
private:
	std::unordered_map<int, trx_t> trxs;
	int trx_cnt;
	lockManager *lm;
	std::mutex trx_manager_latch;
public:
	trxManager();
	int trx_begin(void);
	int trx_commit(int trx_id);
	int trx_abort(int trx_id);
	int trx_abort(trx_t& trx);
	bool dfs(std::unordered_map<int, bool>& visited, trx_t& trx);
	bool is_dead_lock(trx_t& trx);
	void record_lock(int table_id, int64_t key, int trx_id, bool is_write);
	trx_t& operator [](int trx_id);
};

#endif
