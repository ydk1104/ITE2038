#ifndef __BPT_H__
#define __BPT_H__

// Uncomment the line below if you are compiling on Windows.
// #define WINDOWS
#include <file.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
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
typedef struct record {
	char value[120];
} record;

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
typedef struct node {
union{
	void ** pointers;
	pagenum_t *pages;
};
    int64_t * keys;
    pagenum_t parent;
    bool is_leaf;
    int num_keys;
	pagenum_t pagenum; // Used for disk-io.
} node;

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

// Output and utility.

void license_notice( void );
void print_license( int licence_part );
void usage_1( void );
void usage_2( void );
void usage_3( void );
int find_range( pagenum_t root, int64_t key_start, int64_t key_end, bool verbose,
        int64_t returned_keys[], void * returned_pointers[]); 
node * find_leaf( pagenum_t root, int64_t key);
int find( pagenum_t root, int64_t key, char* ret_val);
int cut( int length );

// Insertion.

record * make_record(const char* value);
node * make_node( void );
node * make_leaf( void );
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
pagenum_t start_new_tree(int64_t key, record * pointer);
pagenum_t insert( pagenum_t root, int64_t key, const char* value );

// Deletion.

int get_neighbor_index( node * n );
pagenum_t adjust_root(node * root);
pagenum_t coalesce_nodes(pagenum_t root, node * n, node * neighbor,
                      int neighbor_index, int64_t k_prime);
pagenum_t redistribute_nodes(pagenum_t root, node * n, node * neighbor,
                          int neighbor_index,
        int64_t k_prime_index, int64_t k_prime);
pagenum_t delete_entry( pagenum_t root, node * n, int64_t key, void * pointer );
pagenum_t delete( pagenum_t root, int64_t key );

#endif /* __BPT_H__*/
