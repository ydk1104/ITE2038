#include<lock_table.h>

// #define DEBUG
#ifndef DEBUG
	#define printf(x, ...) (void*)0
#endif

struct lock_t {
	/* NO PAIN, NO GAIN. */
	std::pair<int, int64_t> key;
	lock_t *prev, *next, *head, *tail;
//	std::condition_variable c;
	bool cv;
};

typedef struct lock_t lock_t;
std::condition_variable c;

class my_hash{
public:
	size_t operator()(const std::pair<int, int64_t>& x) const{
		return std::hash<int>{}(x.first) ^ std::hash<int64_t>{}(x.second);
	}
};

std::unordered_map<std::pair<int, int64_t>, lock_t*, my_hash> lock_table;
std::mutex mutex;

int
init_lock_table()
{
	/* DO IMPLEMENT YOUR ART !!!!! */
	lock_table.clear();
	return 0;
}

lock_t*
lock_acquire(int table_id, int64_t key)
{
	/* ENJOY CODING !!!! */
	std::unique_lock<std::mutex> lock(mutex);
	std::pair<int, int64_t> p = {table_id, key};
	lock_t* l = new lock_t; //(lock_t*)malloc(sizeof(lock_t));
	l->key = p;
	//std::unique_lock<std::mutex> lock(mutex);
	printf("acquire %d %ld\n", table_id, key);

	lock_t *&head = lock_table[p];
	if(head == NULL){
		head = new lock_t; //(lock_t*)calloc(sizeof(lock_t), 1);
		head->tail = head;
	}
	head->tail->next = l;
	l->prev = head->tail;
	l->next = NULL;
	head->tail = l;
	l->cv = false;
	if(l != head->next)
		c.wait(lock, [&l]{
						printf("cond %d %ld %d\n", l->key.first, l->key.second, l->cv);
						return l->cv;});
//						printf("cond %d %ld\n", l->key.first, l->key.second);
//						return l==head->next;});
/*		l->c.wait(lock, [&head, &l]{
					printf("cond %d %ld %d\n", l->key.first, l->key.second, l==head->next);
					return l==head->next;}); // */
	/*
	{
		iter->second->tail->next = l;
		l->prev = iter->second->tail;
		iter->second->tail = l;
		l->c.wait(lock, [&iter, &l]{
						printf("cond %d %ld\n", l->key.first, l->key.second);
						return l == iter->second->next;});
	} */
	lock.unlock();
	return l;
}

int
lock_release(lock_t* lock_obj)
{
	/* GOOD LUCK !!! */
	std::unique_lock<std::mutex> lock(mutex);
	printf("release %d %ld\n", lock_obj->key.first, lock_obj->key.second);
	lock_t* head = lock_table[lock_obj->key];
	lock_t* next = lock_obj->next; // == head->next->next;
	if(next){
		head->next = next;
		next->cv = true;
		c.notify_all();
//		next->c.notify_all();
		printf("notify %d %ld %d\n", next->key.first, next->key.second, next->cv);
	}
	else{
		head->next = NULL;
		head->tail = head;
	}
	free(lock_obj);
	lock.unlock();
	return 0;
}
