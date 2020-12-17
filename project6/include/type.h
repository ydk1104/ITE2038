#ifndef __record__
#define __record__
#include <stdint.h>
#include <string.h>
#include <tuple>

struct page_t;
struct node;
class trx_t;
class lock_t;
class lockManager;
class bufferManager;
class fileManager;
class trxManager;
class logManager;

class my_hash{
	public:
		size_t operator()(const std::pair<int, int64_t>& x)const{
			return std::hash<int>{}(x.first) ^ std::hash<int64_t>{}(x.second);
		}
};

enum{
	SHARED_LOCK,
	EXCLUSIVE_LOCK,
};

using pagenum_t = uint64_t;

struct record{
	char value[120];
	record():value{0,}{}
	record(const char* x){
		if(x == NULL) memset(value, 0, 119);
		else strncpy(value, x, 119);
		value[119] = 0;
	}
	void operator =(const record& x){
		for(int i=0; i<120; i++) value[i] = x.value[i];
	}
	void operator =(const char* x){
		if(x == NULL) memset(value, 0, 119);
		else strncpy(value, x, 119);
		value[119] = 0;
	}
};
#endif

