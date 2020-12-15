#include<lock.h>
#include<trx.h>

lockManager::lockManager(trxManager* tm):tm(tm){}
int lockManager::lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode, std::mutex& trx_manager_latch, lock_t* l){
	std::unique_lock<std::mutex> trx_lock(trx_manager_latch);
	auto& trx = (*tm)[trx_id];
	//if trx already acquire lock,
	//don't need to acquire duplication lock
	std::pair<int, int64_t> p = {table_id, key};

//	caller alloc lock_t
//	lock_t* l = new lock_t;
	l->lock_mode = lock_mode;
	l->trx_id = trx_id;
	l->trx = &trx;
	std::unique_lock<std::mutex> lock(lock_manager_latch);
	
	if(trx.lock_acquired(p, lock_mode)){
		//already acquired
		return 0;
	}
	
	//trx has lock_t list because of commit
	trx.add_lock(l);
	
	lock_t* head = &lock_table[p];
	if(head->tail == NULL){
		head->tail = head;
	}
	
	//push_back
	l->prev = head->tail;
	head->tail->next = l;
	head->tail = l;
	l->head = head;

	auto temp = head->x_lock;
	//update x_lock
	if(lock_mode == EXCLUSIVE_LOCK){
		head->x_cnt++;
		head->x_lock = l;
	}

	//test
	if(0)
	{
		auto i = head->next;
		printf("key : %d %ld, ", table_id, key);
		while(i != NULL){
			printf("%d ", i->trx_id);
			i = i->next;
		}
		printf("\n");
	}
	
	if(l->lock_mode == SHARED_LOCK){
		//if lock_mode == shared, check x lock
		if(head->x_cnt == 0) return 0;
		//add edge at last x lock
		//trx.add_edge(head->x_lock->trx_id);
		for(lock_t* i = l->head; i != l; i = i->next){
			if(i->lock_mode == EXCLUSIVE_LOCK) trx.add_edge(i->trx_id);
		}
	}
	else{
		//if lock_mode == exclusive, check no lock
		if(l == head->next) return 0;
		//add edge at front s locks, one x lock
		for(lock_t* i = l->prev; i != head; i = i->prev){
			trx.add_edge(i->trx_id);
//			if(i->lock_mode == EXCLUSIVE_LOCK) break;
		}
	}
	//detect dead_lock
	if(tm->is_dead_lock(trx)){
		head->x_lock = temp;
		return 2;
	}
	//prevent lost wake up
	trx.lock();
	//return wait
	return 1;
}

void lockManager::lock_wait(lock_t* l){
	std::unique_lock<std::mutex>& trx_lock = l->trx->get_trx_lock();
	//if lock_mode == shared, check x lock
	if(l->lock_mode == SHARED_LOCK){
		int cnt = 0;
		//use trx_lock to prevent lost wake up
		l->c.wait(trx_lock, [&cnt, &l]{
			return cnt++;
		});
	}
	else{
		//use trx_lock to prevent lost wake up
		l->c.wait(trx_lock, [&l]{
			auto& head = l->head;
			//head - X1 or head - S1 - X1
			return l == head->next ||
				   (head->next &&
					l == head->next->next &&
					head->next->trx_id == l->trx_id);
		});
	}
	trx_lock.unlock();
	return;
}

void lockManager::lock_release(lock_t* lock_obj){
	if(lock_obj == NULL) return;
	std::unique_lock<std::mutex> lock(lock_manager_latch);
	lock_t *head = lock_obj->head, *next = lock_obj->next;
/* case : 4
 * 1. S->S. impossible
 * 2. S->X. notify if head->S->X or head->S1->S2->X1
 * 3. X->S. notify {}, {} = head->X->{S1,S2,...SN->X}
 * 4. X->X. notify if 3 when N=0
 */
	if(lock_obj->lock_mode == EXCLUSIVE_LOCK) head->x_cnt--;
	lock_obj->prev->next = next;
	if(next) next->prev = lock_obj->prev;
	else head->tail = lock_obj->prev;
	delete lock_obj;
	if(next == head->next){
			{
			lock_t* i = next;
			while(i){
				//need
				std::unique_lock<std::mutex> temp(i->trx->get_trx_mutex());
				// 1 + 3
				// 2 + 4
				// + head-{release S1}-S2-L2
				i->c.notify_one();
				if(i->lock_mode == EXCLUSIVE_LOCK) break;
				i = i->next;
			}
			if(i){
				i->c.notify_one();
			}
		}
	}
}
