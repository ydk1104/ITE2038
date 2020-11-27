#ifndef __record__
#define __record__
#include <string.h>

struct page_t;
struct node;

struct record{
	char value[120];
	record():value{0,}{}
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

