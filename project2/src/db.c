#include<bpt.h>
#include<db.h>
#include<file.h>

int open_table (char *pathname){
	int table_id;
	table_id = file_open(pathname);
	return table_id;
}
int db_insert (int64_t key, char * value){
	page_t header;
	file_read_page(0, &header);
	node* root = page_to_node(header.header.rootPageNum);
	root = insert(root, key, value);
	node_to_page(root);
	file_read_page(0, &header);
	if(root->pagenum != header.header.rootPageNum){
		header.header.rootPageNum = root->pagenum;
		file_write_page(0, &header);
	}
}
int db_find (int64_t key, char * ret_val){
	return 1;
}
int db_delete (int64_t key){
	return 1;
}
