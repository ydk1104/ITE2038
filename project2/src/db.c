#include<db.h>
#include<file.h>

int open_table (char *pathname){
	int table_id;
	table_id = file_open(pathname);
	return table_id;
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
