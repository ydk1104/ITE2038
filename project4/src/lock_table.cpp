#include<lock_table.h>

struct lock_t {
	/* NO PAIN, NO GAIN. */
	lock_t *next=NULL, *head, *tail=NULL;
	std::condition_variable c;
};

class my_hash{
public:
	size_t operator()(const std::pair<int, int64_t>& x) const{
		return std::hash<int>{}(x.first) ^ std::hash<int64_t>{}(x.second);
	}
};

std::unordered_map<std::pair<int, int64_t>, lock_t, my_hash> lock_table;
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
	std::pair<int, int64_t> p = {table_id, key};
	lock_t* l = new lock_t;
	std::unique_lock<std::mutex> lock(mutex);
	lock_t *head = &lock_table[p];
	if(head->tail == NULL)
		head->tail = head;
	//push_back
	head->tail->next = l;
	head->tail = l;
	l->head = head;
	if(l != head->next)
		l->c.wait(lock, [&l, &head]{return l == head->next;});
	return l;
}

int
lock_release(lock_t* lock_obj)
{
	/* GOOD LUCK !!! */
	std::unique_lock<std::mutex> lock(mutex);
	lock_t* head = lock_obj->head;
	lock_t* next = lock_obj->next;
	if(next){
		head->next = next;
		next->c.notify_one();
	}
	else{
		head->next = NULL;
		head->tail = head;
	}
	free(lock_obj);
	return 0;
}
