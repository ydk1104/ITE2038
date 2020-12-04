#ifndef __TRX__
#define __TRX__

#include<lock.h>
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
	std::vector<lock_t*> lock;
	class log_t{
	private:
		int type;
		int table_id;
		int64_t key;
		int trx_id;
		std::shared_ptr<record> value;
	public:
		log_t(int type, int table_id, int key, int trx_id, char* value):
				type(type), table_id(table_id), key(key), trx_id(trx_id), value(value ? std::shared_ptr<record>(new record(value)) : NULL){}
		void undo(){
			switch(type){
				case UPDATE :
					db_undo_update(table_id, key, value->value, trx_id);
					break;
				case FIND :
				default:
					break;
			}
		}
	};
	std::vector<log_t> logs;
	std::unordered_map<std::pair<int, int64_t>, lock_t*, my_hash> acquired_lock;	bool aborted = false;
public:
	trx_t():trx_t(0){}
	trx_t(int trx_id):trx_id(trx_id){}
	const int get_trx_id()const{return trx_id;}
	void end(lockManager* lm){
		for(auto i:acquired_lock) lm->lock_release(i.second);
	}
	void commit(lockManager* lm){
		end(lm);
	}
	void abort(lockManager* lm){
		aborted = true;
		for(auto& i:logs){
			i.undo();
		}
		end(lm);
	}
	void add_log(int type, int table_id, int64_t key, char* value){
		if(!aborted)
			logs.emplace_back(type, table_id, key, trx_id, value);
	}
	void add_edge(int x){
		if(x==trx_id) return; // self loop is an-available;
		printf("%x<-%x\n", x, trx_id);
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
	bool lock_acquired(std::pair<int, int64_t>& key, lock_t* lock){
		auto& acquired_lock_ = acquired_lock[key];
		if(acquired_lock_ && 
			acquired_lock_->lock_mode < lock->lock_mode){

			lock_t* next = acquired_lock_->next;
			lock_t* head = acquired_lock_->head;
			acquired_lock_->prev->next = next;
			if(next) next->prev = acquired_lock_->prev;
			else head->tail = acquired_lock_->prev;
			delete acquired_lock_;

			acquired_lock_ = lock;
			return false;
		}
		if(acquired_lock_ == NULL){
			acquired_lock_ = lock;
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
	bool dfs(std::unordered_map<int, bool>& visited, trx_t& trx, int start_id);
	bool is_dead_lock(trx_t& trx);
	bool record_lock(int table_id, int64_t key, int trx_id, bool is_write);
	bool find(int trx_id);
	void logging(int type, int table_id, int64_t key, char* value, int trx_id);
	trx_t& operator [](int trx_id);
};

#endif

