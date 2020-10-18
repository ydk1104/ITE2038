#include "bpt.h"
#include "db.h"
#include "file.h"

// MAIN

void insert_test(int N){
	char s[11] = "0123456789";
	char val[120];
	for(int i=0; i<N; i++){
		int error = db_insert(i, s+(i%10));
		printf("insert test : %d\n", i);
		if(error) printf("FAILED");
	}
	for(int i=0; i<N; i++){
		int error = db_find(i, val);
		printf("find test : %d %s\n", i, val);
		if(error) printf("FAILED");
	}
	for(int i=0; i<N; i++){
		int error = db_delete(i);
		printf("delete test : %d\n", i);
		if(error) printf("FAILED");
	}
	for(int i=0; i<N; i++){
		int error = db_find(i, val);
		printf("not find test : %d", i);
		if(!error) printf("FAILED");
	}
}

int my_main(){
//	int tbl_id = open_table("out/out.txt");
	int tlb_id = open_table("/mnt/ramdisk/out.txt");
	page_t head = {0, };
	file_read_page(0, &head);
	printf("header page:%lx %lx %lx\n",
					head.header.freePageNum,
					head.header.rootPageNum,
					head.header.numOfPages);
	int N = 1e5;
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
