#include<lock.h>
#include<trx.h>

lockManager::lockManager(trxManager* tm):tm(tm){}
lock_t* lockManager::lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode, std::mutex& trx_manager_latch){
	std::pair<int, int64_t> p = {table_id, key};
	lock_t* l = new lock_t;
	l->lock_mode = lock_mode;
	l->trx_id = trx_id;
	std::unique_lock<std::mutex> trx_lock(trx_manager_latch);
	std::unique_lock<std::mutex> lock(lock_manager_latch);

	lock_t* head = &lock_table[p];
	if(head->tail == NULL){
		head->tail = head;
	}

	auto& trx = (*tm)[trx_id];
	//if trx already acquire lock,
	//don't need to acquire duplication lock
	if(trx.lock_acquired(lock_mode)) return nullptr;

	//trx has lock_t list because of commit
	trx.add_lock(l);

	
	//push_back
	l->prev = head->tail;
	head->tail->next = l;
	head->tail = l;
	l->head = head;

	//update x_lock
	head->x_cnt++;
	head->x_lock = l;
	
	//if lock_mode == exclusive, check no lock
	if(l != head->next){
		//if lock_mode == shared, check x lock
		if(l->lock_mode == SHARED_LOCK){
			if(head->x_cnt == 0) return l;
			int cnt = 0;
			//add edge at last x lock
			trx.add_edge(head->x_lock->trx_id);
			//detect dead_lock
			if(tm->is_dead_lock(trx)){
				tm->trx_abort(trx);
				return NULL;
			}

			//unlock trx_lock before wait
			//don't need to re-lock
			trx_lock.unlock();
			l->c.wait(lock, [&cnt]{
				return cnt++;
			});
		}
		else{
			//add edge at front s locks, one x lock
			for(lock_t* i = l->prev; i != head; i = i->prev){
				trx.add_edge(i->trx_id);
				if(i->lock_mode == EXCLUSIVE_LOCK) break;
			}
			//detect dead_lock
			if(tm->is_dead_lock(trx)){
				tm->trx_abort(trx);
				return NULL;
			}

			//unlock trx_lock before wait
			//don't need to re-lock
			trx_lock.unlock();
			l->c.wait(lock, [&l, &head]{
				return l == head->next;
			});
		}
	}
	return l;
}
void lockManager::lock_release(lock_t* lock_obj){
	std::unique_lock<std::mutex> lock(lock_manager_latch);
	lock_t *head = lock_obj->head, *next = lock_obj->next;
/* case : 4
 * 1. S->S. impossible
 * 2. S->X. notify if head->S->X.
 * 3. X->S. notify {}, {} = head->X->{S1,S2,...SN}->X
 * 4. X->X. notify if 3 when N==0
 */
	if(lock_obj == head->next){
		if(next == NULL){
			head->next = NULL;
			head->tail = head;
			goto end;
		}
		if(next->lock_mode == EXCLUSIVE_LOCK){
			next->c.notify_one();
		}
		else{
			lock_t* i = next;
			while(i && i->lock_mode == SHARED_LOCK){
				i->c.notify_one();
				i = i->next;
			}
		}
	}
	lock_obj->prev->next = next;
	if(next) next->prev = lock_obj->prev;
end:
	if(lock_obj->lock_mode == EXCLUSIVE_LOCK) head->x_cnt--;
	delete lock_obj;
}
