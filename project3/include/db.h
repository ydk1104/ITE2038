#ifndef __db__
#define __db__

#include<stdint.h>
int init_db (int buf_num);
int open_table (char *pathname);
int db_insert (int table_id, int64_t key, char * value);
int db_find (int table_id, int64_t key, char * ret_val);
int db_delete (int table_id, int64_t key);
int close_table(int table_id);
int shutdown_db(void);

#endif
