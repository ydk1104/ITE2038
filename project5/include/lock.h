#ifndef __LOCK__
#define __LOCK__

#include<condition_variable>
#include<mutex>
#include<type.h>
#include<unordered_map>

class lock_t{
public:
	lock_t *next, *prev, *head, *tail, *x_lock;
	std::condition_variable c;
	int x_cnt, lock_mode, trx_id;
	trx_t* trx;
	lock_t():next(NULL),tail(NULL),x_cnt(0),lock_mode(-1){}
};

class lockManager{
private:
	std::unordered_map<std::pair<int, int64_t>, lock_t, my_hash> lock_table;
	std::mutex lock_manager_latch;
	trxManager* tm;
public:
	lockManager(trxManager* tm);	
	int lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode, std::mutex& trx_manager_latch, lock_t* l);
	void lock_wait(lock_t* l);
	void lock_release(lock_t* lock_obj);
};

#endif
