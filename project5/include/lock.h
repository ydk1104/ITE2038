#ifndef __LOCK__
#define __LOCK__

#include<stdint.h>

class lock_t{
};

class lockManager{
private:
	
public:
	lock_t* lock_acquire(int table_id, int64_t key, int trx_id, int lock_mode);
	void lock_release(lock_t* lock_obj);
};

#endif
