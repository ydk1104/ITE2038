#include "bpt.h"
#include "db.h"
#include "file.h"
#include <vector>
#include <thread>

// MAIN

#define PRINT
#ifndef PRINT 
	#define printf(x, ...) (void*)(x)
#endif

void insert_test(int N, int table_id){
	N /= 10;
	char s[11] = "0123456789";
	char val[120];
	int64_t offset = 0;
/*	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_insert(table_id, key, s+(i%10));
		printf("insert test : %ld\n", key);
		if(error) printf("FAILED"), exit(-1);
	}// */
	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_find(table_id, key, val, 1);
		printf("find test : %ld %s\n", key, val);
		if(error) printf("FAILED"), exit(-1);
	} // */
/*	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_delete(table_id, key);
		printf("delete test : %ld\n", key);
		if(error) printf("FAILED"), exit(-1);
	}
	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_find(table_id, key, val);
		printf("not find test : %ld\n", key);
		if(!error) printf("FAILED"), exit(-1);
	} // */
}

void find_test(int N, int table_id){
	N /= 10;
	char s[11] = "0123456789";
	char val[120];
	int64_t offset = 0;
	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_find(table_id, key, val, 1);
		printf("find test : %ld %s\n", key, val);
		if(error) printf("FAILED"), exit(-1);
	}
}

int open_table_test(void){
	char s[TABLE_SIZE][101] = {
		"out/test_open0.txt",
		"out/test_open1.txt",
		"out/test_open2.txt",
		"out/test_open3.txt",
		"out/test_open4.txt",
		"out/test_open5.txt",
		"out/test_open6.txt",
		"out/test_open7.txt",
		"out/test_open8.txt",
		"out/test_open9.txt",
	};
	int tbl_id;
	for(int i=0; i<TABLE_SIZE; i++){
		tbl_id = open_table(s[i]);
		if(tbl_id != i+1){
			printf("open %s, tbl_id = %d\n", s[i], tbl_id);
			goto err;
		}
	}
	tbl_id = open_table("out/fail.txt");
	if(tbl_id != -1){
		printf("open fail, tbl_id = %d\n", tbl_id);
		goto err;
	}
	for(int i=0; i<TABLE_SIZE; i++){
		tbl_id = open_table(s[i]);
		if(tbl_id != i+1){
			printf("reopen %s, tbl_id = %d\n", s[i], tbl_id);
			goto err;
		}
	}

	printf("success open_table_test\n");
	return 0;
err:
	printf("open_table_test failed\n");
	exit(-1);
}

typedef enum{
	TEST_OPEN,
	TEST_RAM_INSERT,
	TEST_DISK_INSERT,
}TEST;

void find_multi_thread(int N, int s, int e){
	std::thread find_threads[10];
	for(int i=s; i<e; i++){
		find_threads[i] = (std::thread(find_test, N, i));
	}
	for(int i=s; i<e; i++){
		find_threads[i].join();
	}
	
}

void test(TEST test){
	if(test == TEST_OPEN){
		open_table_test();
		return;
	}
	int tbl_id;
	open_table("/mnt/ramdisk/out2.txt");
	if(test == TEST_RAM_INSERT)
		tbl_id = open_table("/mnt/ramdisk/out.txt");
	else
		tbl_id = open_table("out/out.txt");
	int N = 1e6;
	find_multi_thread(N, 1, 3);
//	find_test(N, 1);
//	close_table(1);
//	find_test(N, 2);
//	close_table(2);
}

int my_main(){
	const int buff_size = 10000;
	init_db(buff_size);
	TEST type = TEST_RAM_INSERT;
	test(type);
	shutdown_db();
	return 0;
}

int main( int argc, char ** argv ) {
	return my_main();
}
