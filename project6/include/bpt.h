#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <buffer.h>
#include <type.h>
#include <file.h>
#include <trx.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#ifdef WINDOWS
#define bool char
#define false 0
#define true 1
#endif

// Default order is 4.
#define DEFAULT_ORDER 4
#define DEFAULT_LEAF_ORDER 32
#define DEFAULT_INTERNAL_ORDER 249


// Minimum order is necessarily 3.  We set the maximum
// order arbitrarily.  You may change the maximum order.
#define MIN_ORDER 3
#define MAX_ORDER 20

// Constants for printing part or all of the GPL license.
#define LICENSE_FILE "LICENSE.txt"
#define LICENSE_WARRANTEE 0
#define LICENSE_WARRANTEE_START 592
#define LICENSE_WARRANTEE_END 624
#define LICENSE_CONDITIONS 1
#define LICENSE_CONDITIONS_START 70
#define LICENSE_CONDITIONS_END 625

// TYPES.

/* Type representing the record
 * to which a given key refers.
 * In a real B+ tree system, the
 * record would hold data (in a database)
 * or a file (in an operating system)
 * or some other information.
 * Users can rewrite this part of the code
 * to change the type and content
 * of the value field.
 */


/* Type representing a node in the B+ tree.
 * This type is general enough to serve for both
 * the leaf and the internal node.
 * The heart of the node is the array
 * of keys and the array of corresponding
 * pointers.  The relation between keys
 * and pointers differs between leaves and
 * internal nodes.  In a leaf, the index
 * of each key equals the index of its corresponding
 * pointer, with a maximum of order - 1 key-pointer
 * pairs.  The last pointer points to the
 * leaf to the right (or NULL in the case
 * of the rightmost leaf).
 * In an internal node, the first pointer
 * refers to lower nodes with keys less than
 * the smallest key in the keys array.  Then,
 * with indices i starting at 0, the pointer
 * at i + 1 points to the subtree with keys
 * greater than or equal to the key in this
 * node at index i.
 * The num_keys field is used to keep
 * track of the number of valid keys.
 * In an internal node, the number of valid
 * pointers is always num_keys + 1.
 * In a leaf, the number of valid pointers
 * to data is always num_keys.  The
 * last leaf pointer points to the next leaf.
 */

typedef uint64_t pagenum_t;
struct node {
union{
//	void ** pointers;
//	pagenum_t *pages;

	class{
	public:
		page_t* buffer_ptr;
		record& operator [](int i){
			return buffer_ptr->data.pageData.leaf[i].value;
		}
	}pointers;

	class{
	public:
		page_t* buffer_ptr;
		pagenum_t& operator [](int i){
			auto& temp = buffer_ptr->data.pageData;
			if(temp.page.isLeaf){
				if(i == DEFAULT_LEAF_ORDER - 1) return buffer_ptr->data.pageData.page.pageNum;
			}
			if(i==0) return temp.page.pageNum;
			return temp.internal[i-1].pageNum;
		}
	}pages;
};
//    int64_t * keys;
	class{
	public:
		page_t* buffer_ptr;
		int64_t& operator [](int i){
			if(buffer_ptr->data.pageData.page.isLeaf) return buffer_ptr->data.pageData.leaf[i].key;
			return buffer_ptr->data.pageData.internal[i].key;
		}
	}keys;
    pagenum_t& parent;
    uint32_t& is_leaf;
    uint32_t& num_keys;
	int64_t& pageLSN;

	int table_id;
	pagenum_t pagenum; // Used for disk-io.
	struct page_t* buffer_ptr; // Used for --buffet_ptr->is_pinned.
	node(page_t* page_ptr):
		node(page_ptr, page_ptr->pagenum, page_ptr->table_id,
			 page_ptr->data.pageData.page.parentPageNum,
			 page_ptr->data.pageData.page.isLeaf,
			 page_ptr->data.pageData.page.numOfKeys,
			 page_ptr->data.pageData.page.pageLSN){}
	node(page_t* buffer_ptr, pagenum_t pagenum, int table_id, pagenum_t& parent, uint32_t& is_leaf, uint32_t& num_keys, int64_t& pageLSN):
			buffer_ptr(buffer_ptr),
			pagenum(pagenum),
			table_id(table_id),
			parent(parent),
			is_leaf(is_leaf),
			num_keys(num_keys),
			pageLSN(pageLSN){
				pointers.buffer_ptr = pages.buffer_ptr = keys.buffer_ptr = buffer_ptr;
			}
};

// GLOBALS.

/* The order determines the maximum and minimum
 * number of entries (keys and pointers) in any
 * node.  Every node has at most order - 1 keys and
 * at least (roughly speaking) half that number.
 * Every leaf has as many pointers to data as keys,
 * and every internal node has one more pointer
 * to a subtree than the number of keys.
 * This global variable is initialized to the
 * default value.
 */
extern int leaf_order, internal_order;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
extern bool verbose_output;


// FUNCTION PROTOTYPES.

// layer.
int init_bpt(bufferManager* bm, trxManager* tm);
int close_buffer(int table_id);
int shutdown_buffer(void);
int file_open(char* pathname);
page_t* file_alloc_page(int table_id);
void file_free_page(int table_id, pagenum_t pagenum);
page_t* get_header_ptr(int table_id, bool is_read);

node * find_leaf( int table_id, pagenum_t root, int64_t key);
int find_record( node* c);
int find( int table_id, pagenum_t root, int64_t key, char* ret_val, int trx_id);
int update( int table_id, pagenum_t root, int64_t key, char* values, int trx_id);
int cut( int length );

// Insertion.

record * make_record(const char* value);
node * make_node( int table_id );
node * make_leaf( int table_id );
int get_left_index(node * parent, pagenum_t pagenum);
node * insert_into_leaf( node * leaf, int64_t key, record * pointer );
pagenum_t insert_into_leaf_after_splitting(pagenum_t root, node * leaf, int64_t key,
                                        record * pointer);
pagenum_t insert_into_node(pagenum_t root, node * parent, 
        int left_index, int64_t key, pagenum_t right_pagenum);
pagenum_t insert_into_node_after_splitting(pagenum_t root, node * parent,
                                        int left_index,
        int64_t key, pagenum_t right_pagenum);
pagenum_t insert_into_parent(pagenum_t root, node * left, int64_t key, node * right);
pagenum_t insert_into_new_root(node * left, int64_t key, node * right);
pagenum_t start_new_tree(int table_id, int64_t key, record * pointer);
pagenum_t insert( int table_id, pagenum_t root, int64_t key, const char* value );

// Deletion.

int get_neighbor_index( node * n );
pagenum_t adjust_root(node * root);
pagenum_t coalesce_nodes( pagenum_t root, node * n, node * neighbor,
                      int neighbor_index, int64_t k_prime);
pagenum_t redistribute_nodes( pagenum_t root, node * n, node * neighbor,
                          int neighbor_index,
        int64_t k_prime_index, int64_t k_prime);
pagenum_t delete_entry( pagenum_t root, node * n, int64_t key, void * pointer );
pagenum_t delete_main( int table_id, pagenum_t root, int64_t key );

#endif /* __BPT_H__*/
