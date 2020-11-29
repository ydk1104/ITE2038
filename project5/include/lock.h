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
	lock_t():next(NULL),tail(NULL),x_cnt(0),lock_mode(-1){}
};

class lockManager{
private:
	class my_hash{
		public:
			size_t operator()(const std::pair<int, int64_t>& x)const{
				return std::hash<int>{}(x.first) ^ std::hash<int64_t>{}(x.second);
			}
	};
	std::unordered_map<std::pair<int, int64_t>, lock_t, my_hash> lock_table;
	std::mutex lock_manager_latch;
	trxManager* tm;
public:
	lockManager(trxManager* tm);	
	lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode, std::mutex& trx_manager_latch);
	void lock_release(lock_t* lock_obj);
};

#endif
