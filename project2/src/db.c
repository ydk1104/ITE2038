#include<db.h>
#include<file.h>

int open_table (char *pathname){
	int file = open(pathname, O_RDWR | O_CREAT | O_SYNC, 666);
	return file;
}
int db_insert (int64_t key, char * value){
	return 1;
}
int db_find (int64_t key, char * ret_val){
	return 1;
}
int db_delete (int64_t key){
	return 1;
}
