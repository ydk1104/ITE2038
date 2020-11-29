#ifndef __TRX__
#define __TRX__

#include"lock.h"
#include<list>
#include<mutex>
#include<unordered_map>
#include<vector>

class trx{
private:
	int trx_id;
	std::list<int> edge;
	std::vector<lock_t*> lock;
	int log[1];
// log;

public:
	void commit(lockManager* lm){
		for(auto i:lock) lm->lock_release(i);
	}
	void abort(void){
		for(auto i:log);
	}
	void add_egde(int x){
		edge.push_back(x);
	}
};

class trxManager{
private:
	std::unordered_map<int, trx> trxs;
	int trx_cnt;
	lockManager *lm;
	std::mutex trx_manager_latch;
public:
	trxManager();
	int trx_begin(void);
	int trx_commit(int trx_id);
	int trx_abort(int trx_id);
	bool isDeadLock();
	void record_lock(int table_id, int64_t key, int trx_id, bool is_write);
};

#endif
