#include "bpt.h"
#include "db.h"
#include "file.h"

// MAIN

//#define PRINT
#ifndef PRINT 
	#define printf(x, ...) (void*)(x)
#endif

void insert_test(int N){
	N /= 10;
	char s[11] = "0123456789";
	char val[120];
	int64_t offset = -5e9;
	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_insert(0, key, s+(i%10));
		printf("insert test : %ld\n", key);
		if(error) printf("FAILED"), exit(-1);
	}// */
	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_find(0, key, val);
		printf("find test : %ld %s\n", key, val);
		if(error) printf("FAILED"), exit(-1);
	} // */
	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_delete(0, key);
		printf("delete test : %ld\n", key);
		if(error) printf("FAILED"), exit(-1);
	}
	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_find(0, key, val);
		printf("not find test : %ld\n", key);
		if(!error) printf("FAILED"), exit(-1);
	} // */
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

void test(TEST test){
	if(test == TEST_OPEN){
		open_table_test();
		return;
	}
	int tbl_id;
	open_table("out/out.txt");
	if(test == TEST_RAM_INSERT)
		tbl_id = open_table("/mnt/ramdisk/out.txt");
	else
		tbl_id = open_table("out/out.txt");
#ifdef PRINT
	page_t* head = get_header_ptr(true);
	head->pin_count--;
	printf("header page:%lx %lx %lx\n",
					head->header.freePageNum,
					head->header.rootPageNum,
					head->header.numOfPages);
#endif
	int N = 1e6;
	insert_test(N);
}

int my_main(){
	const int buff_size = 1000;
	init_db(buff_size);
	TEST type = TEST_RAM_INSERT;
	test(type);
	shutdown_db();
	return 0;
}

int main( int argc, char ** argv ) {

		return my_main();

    char * input_file;
    FILE * fp;
    pagenum_t root;
    int input, range2;
    char instruction;
    char license_part;
	char input_value[120];

    root = 0;
    verbose_output = false;

    license_notice();
    usage_1();  
    usage_2();

    if (argc > 2) {
        input_file = argv[2];
        fp = fopen(input_file, "r");
        if (fp == NULL) {
            perror("Failure  open input file.");
            exit(EXIT_FAILURE);
        }
        while (!feof(fp)) {
            fscanf(fp, "%d %s", &input, input_value);
            root = insert(root, input, input_value);
        }
        fclose(fp);
    }

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%d", &input);
            root = delete(root, input);
            break;
        case 'i':
            scanf("%d %s", &input, input_value);
            root = insert(root, input, input_value);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 'v':
            verbose_output = !verbose_output;
            break;
        default:
            usage_2();
            break;
        }
        while (getchar() != (int)'\n');
        printf("> ");
    }
    printf("\n");

    return EXIT_SUCCESS;
}
