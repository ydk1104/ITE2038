#include "bpt.h"
#include "db.h"
#include "file.h"

// MAIN

void insert_test(int N){
	char s[11] = "0123456789";
	for(int i=0; i<N; i++){
		db_insert(i, s+(i%10));
	}
}

int my_main(){
	int tbl_id = open_table("out/out.txt");
	page_t head = {0, };
	file_read_page(0, &head);
	printf("header page:%lx %lx %lx\n",
					head.header.freePageNum,
					head.header.rootPageNum,
					head.header.numOfPages);
	insert_test(10000);	
	return 0;
}

int main( int argc, char ** argv ) {

		return my_main();

    char * input_file;
    FILE * fp;
    node * root;
    int input, range2;
    char instruction;
    char license_part;
	char input_value[120];

    root = NULL;
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
        print_tree(root);
    }

    printf("> ");
    while (scanf("%c", &instruction) != EOF) {
        switch (instruction) {
        case 'd':
            scanf("%d", &input);
            root = delete(root, input);
            print_tree(root);
            break;
        case 'i':
            scanf("%d %s", &input, input_value);
            root = insert(root, input, input_value);
            print_tree(root);
            break;
        case 'f':
        case 'p':
            scanf("%d", &input);
            find_and_print(root, input, instruction == 'p');
            break;
        case 'r':
            scanf("%d %d", &input, &range2);
            if (input > range2) {
                int tmp = range2;
                range2 = input;
                input = tmp;
            }
            find_and_print_range(root, input, range2, instruction == 'p');
            break;
        case 'l':
            print_leaves(root);
            break;
        case 'q':
            while (getchar() != (int)'\n');
            return EXIT_SUCCESS;
            break;
        case 't':
            print_tree(root);
            break;
        case 'v':
            verbose_output = !verbose_output;
            break;
        case 'x':
            if (root)
                root = destroy_tree(root);
            print_tree(root);
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
