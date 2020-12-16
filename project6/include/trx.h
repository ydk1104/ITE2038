#ifndef __TRX__
#define __TRX__

#include<lock.h>
#include<log.h>
#include<db.h>
#include<list>
#include<memory>
#include<mutex>
#include<unordered_map>
#include<vector>

class trx_t{
private:
	const int trx_id;
	std::list<int> edge;
	std::vector<lock_t*> locks;
	std::mutex trx_latch;
	std::unique_lock<std::mutex> trx_lock;
	std::vector<log_t*> logs;
	logManager* lm;
	int64_t prev_lsn;
	std::unordered_map<std::pair<int, int64_t>, int, my_hash> acquired_lock;
	bool aborted = false;
public:
	trx_t():trx_t(0, nullptr){}
	trx_t(int trx_id, logManager* lm):trx_id(trx_id),trx_lock(trx_latch, std::defer_lock),lm(lm){add_log(BEGIN);}
	const int get_trx_id()const{return trx_id;}
	void end(lockManager* lm, int32_t type){
		for(auto& i:locks) lm->lock_release(i);
		add_log(type);
		flush_log();
		delete logs.back();
	}
	void commit(lockManager* lm){
		end(lm, COMMIT);
	}
	void abort(lockManager* lm, bufferManager* bm){
		aborted = true;
		for(auto& i:logs){
			i->undo(bm);
			delete i;
		}
		end(lm, ROLLBACK);
	}
	void add_log(int32_t type){
		logs.emplace_back(lm->make_log_t(prev_lsn, trx_id, type));
		prev_lsn = logs.back()->get_lsn();
	}
	void flush_log(){
		lm->flush();
	}
	//return prev_lsn == now add log's lsn
	//and update prev_lsn
	int64_t add_log(int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image){
		logs.emplace_back(lm->make_log_t(prev_lsn, trx_id, type, table_id, pageNum, offset, old_image, new_image));
		return prev_lsn = logs.back()->get_lsn();
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
		locks.push_back(lock_obj);
	}
	bool lock_acquired(std::pair<int, int64_t>& key, int lock_mode){
		int& acquired_lock_ = acquired_lock[key];
		if(acquired_lock_ <= lock_mode){
			acquired_lock_ = lock_mode + 1;
			return false;
		}
		return true;
	}
	void lock(){trx_lock.lock();}
	void unlock(){trx_lock.unlock();}
	std::unique_lock<std::mutex>& get_trx_lock(){return trx_lock;}
	std::mutex& get_trx_mutex(){return trx_latch;}
};

class trxManager{
private:
	std::unordered_map<int, trx_t> trxs;
	int trx_cnt;
	lockManager *lm;
	logManager *logMng;
	std::mutex trx_manager_latch;
public:
	trxManager(logManager* logMng);
	int trx_begin(void);
	int trx_commit(int trx_id);
	int trx_abort(int trx_id, bufferManager* bm);
	int trx_abort(trx_t& trx, bufferManager* bm);
	bool dfs(std::unordered_map<int, bool>& visited, trx_t& trx, int start_id);
	bool is_dead_lock(trx_t& trx);
	int record_lock(int table_id, int64_t key, int trx_id, bool is_write, lock_t* l);
	void record_lock_wait(lock_t* l);
	bool find(int trx_id);
	int64_t logging(int32_t trx_id, int32_t type, int32_t table_id, pagenum_t pageNum, int32_t offset, char* old_image, char* new_image);
	trx_t& operator [](int trx_id);
};

#endif
