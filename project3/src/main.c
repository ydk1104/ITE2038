#include "bpt.h"
#include "db.h"
#include "file.h"

// MAIN

#define PRINT
#ifndef PRINT 
	#define printf(x, ...) (void*)(x)
#endif

void insert_test(int N){
	N *= 2;
	char s[11] = "0123456789";
	char val[120];
	int64_t offset = -5e9;
/*	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_insert(key, s+(i%10));
		printf("insert test : %ld\n", key);
		if(error) printf("FAILED"), exit(-1);
	}// */
/*	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_find(key, val);
		printf("find test : %ld %s\n", key, val);
		if(error) printf("FAILED"), exit(-1);
	} // */
	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_delete(key);
		printf("delete test : %ld\n", key);
		if(error) printf("FAILED"), exit(-1);
	}
	for(int i=0; i<N; i++){
		int64_t key = offset+i;
		int error = db_find(key, val);
		printf("not find test : %ld\n", key);
		if(!error) printf("FAILED"), exit(-1);
	} // */
}

int my_main(){
	int tbl_id = open_table("out/out.txt");
//	int tlb_id = open_table("/mnt/ramdisk/out.txt");
//	int tbl_id = open_table("/mnt/ramdisk/t.db");
	page_t head = {0, };
	file_read_page(0, &head);
	printf("header page:%lx %lx %lx\n",
					head.header.freePageNum,
					head.header.rootPageNum,
					head.header.numOfPages);
	int N = 1e6;
	insert_test(N);
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
