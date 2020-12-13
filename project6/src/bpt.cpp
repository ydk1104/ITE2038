/*
 *  bpt.c  
 */
#define Version "1.14"
/*
 *
 *  bpt:  B+ Tree Implementation
 *  Copyright (C) 2010-2016  Amittai Aviram  http://www.amittai.com
 *  All rights reserved.
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions are met:
 *
 *  1. Redistributions of source code must retain the above copyright notice, 
 *  this list of conditions and the following disclaimer.
 *
 *  2. Redistributions in binary form must reproduce the above copyright notice, 
 *  this list of conditions and the following disclaimer in the documentation 
 *  and/or other materials provided with the distribution.
 
 *  3. Neither the name of the copyright holder nor the names of its 
 *  contributors may be used to endorse or promote products derived from this 
 *  software without specific prior written permission.
 
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 *  AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE 
 *  IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE 
 *  ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE 
 *  LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR 
 *  CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF 
 *  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS 
 *  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN 
 *  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) 
 *  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE 
 *  POSSIBILITY OF SUCH DAMAGE.
 
 *  Author:  Amittai Aviram 
 *    http://www.amittai.com
 *    amittai.aviram@gmail.edu or afa13@columbia.edu
 *  Original Date:  26 June 2010
 *  Last modified: 17 June 2016
 *
 *  This implementation demonstrates the B+ tree data structure
 *  for educational purposes, includin insertion, deletion, search, and display
 *  of the search path, the leaves, or the whole tree.
 *  
 *  Must be compiled with a C99-compliant C compiler such as the latest GCC.
 *
 *  Usage:  bpt [order]
 *  where order is an optional argument
 *  (integer MIN_ORDER <= order <= MAX_ORDER)
 *  defined as the maximal number of pointers in any node.
 *
 */

#include "bpt.h"

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
int internal_order = DEFAULT_INTERNAL_ORDER, leaf_order = DEFAULT_LEAF_ORDER;

/* The user can toggle on and off the "verbose"
 * property, which causes the pointer addresses
 * to be printed out in hexadecimal notation
 * next to their corresponding keys.
 */
bool verbose_output = false;

bufferManager* bm;
trxManager* tm;

int init_bpt(int buf_num, trxManager* tm_ptr){
	tm = tm_ptr;
	bm = new bufferManager(buf_num);
	return 0;
}

int close_buffer(int table_id){
	return bm->close_buffer(table_id);
}

int shutdown_buffer(void){
	delete bm;
	return 0;
}

int file_open(char* pathname){
	return bm->file_open(pathname);
}

page_t* file_alloc_page(int table_id){
	return bm->file_alloc_page(table_id);
}

void file_free_page(int table_id, pagenum_t pagenum){
	return bm->file_free_page(table_id, pagenum);
}

page_t* get_header_ptr(int table_id, bool is_read){
	return bm->get_header_ptr(table_id, is_read);
}

// FUNCTION DEFINITIONS.

void free_node(node** node_ptr){
	node* node = *node_ptr;
	node->buffer_ptr->unlock();
	delete *node_ptr;
	*node_ptr = NULL;
}

void node_to_page(node** nptr, bool doFree){
	bm->node_to_page(nptr, doFree);
}
void page_to_node(int table_id, pagenum_t pagenum, node** nptr){
	bm->page_to_node(table_id, pagenum, nptr);
}

/* Traces the path from the root to a leaf, searching
 * by key.  Displays information about the path
 * if the verbose flag is set.
 * Returns the leaf containing the given key.
 */
node * find_leaf( int table_id, pagenum_t root, int64_t key) {
    int i = 0;
    node * c = NULL;
	page_to_node(table_id, root, &c);
    if (c == NULL) {
        return c;
    }
    while (c && !c->is_leaf) {
        i = 0;
        while (i < c->num_keys) {
            if (key >= c->keys[i]) i++;
            else break;
        }
        page_to_node(table_id, c->pages[i], &c);
    }
    return c;
}


/* Finds and returns the record to which
 * a key refers.
 */
int find_record(node* c, int64_t key) {
    int i = 0;
	//can't find leaf
    if (c == NULL) return -1;
	//return record idx
    for (i = 0; i < c->num_keys; i++)
        if (c->keys[i] == key) return i;
	//can't find record
	return -1;
}

node* record_lock_acquire( int table_id, pagenum_t root, int64_t key, int trx_id, int lock_mode, int& ret_idx){
	node* c = find_leaf(table_id, root, key);
	ret_idx = find_record(c, key);
	//can't find leaf or record
	if(ret_idx == -1){
		//check NULL
		if(c) free_node(&c);
		return NULL;
	}
	lock_t* l = new lock_t;
	int ret = tm->record_lock(table_id, key, trx_id, lock_mode, l);
	printf("ret : %d\n", ret);
	switch(ret){
		//acquire, page lock & record lock
		case 0 :
			break;
		//wait
		case 1 :
			//release page lock
			free_node(&c);
			//already lock trx latch, prevent lost wake_up
			tm->record_lock_wait(l);
			//SLOW! TODO
			//before find_leaf, check c is not eviction TODO
			//c = page_to_node(pagenum), check c->pointers[ret_idx].key == key
			c = find_leaf(table_id, root, key);
			break;
		//deadlock
		case 2 :
		default :
			free_node(&c);
			return NULL;
	}
	return c;
}

int find( int table_id, pagenum_t root, int64_t key, char* ret_val, int trx_id){
	
	int record_idx;
	//acquire shared lock
	node* leaf = record_lock_acquire(table_id, root, key, trx_id, 0, record_idx);
	printf("%p\n", leaf);
	if(leaf == NULL) return 1;
	strncpy(ret_val, leaf->pointers[record_idx].value, 120);
	free_node(&leaf);
	return 0;
}

int update( int table_id, pagenum_t root, int64_t key, char* values, int trx_id, bool undo){
	int record_idx;
	node* leaf;
	//acquire exclusive lock TODO : undo don't acquire lock
	if(!undo){
		leaf = record_lock_acquire(table_id, root, key, trx_id, 1, record_idx);
		printf("%p\n", leaf);
		if(leaf == NULL) return 1;
	}
	else{
		leaf = find_leaf(table_id, root, key);
		record_idx = find_record(leaf, key);
	}
/*	if(!undo){
		if(!tm->record_lock(table_id, key, trx_id, true)) return 1;
	}*/
	strncpy(leaf->pointers[record_idx].value, values, 120);
	// logging
	if(!undo) tm->logging(UPDATE, table_id, key, values, trx_id);
	node_to_page(&leaf, true);
	return 0;
}

/* Finds the appropriate place to
 * split a node that is too big into two.
 */
int cut( int length ) {
    if (length % 2 == 0)
        return length/2;
    else
        return length/2 + 1;
}


// INSERTION

/* Creates a new general node, which can be adapted
 * to serve as either a leaf or an internal node.
 */
node * make_node( int table_id ) {
	node * new_node = new node(file_alloc_page(table_id));
   
	if (new_node == NULL) {
        perror("Node creation.");
        exit(EXIT_FAILURE);
    }
    
	new_node->is_leaf = false;
    new_node->num_keys = 0;
    new_node->parent = 0;

	return new_node;
}

/* Creates a new leaf by creating a node
 * and then adapting it appropriately.
 */
node * make_leaf( int table_id ) {
    node * leaf = make_node(table_id);
	leaf->is_leaf = true;
    return leaf;
}


/* Helper function used in insert_into_parent
 * to find the index of the parent's pagenum to 
 * the node to the left of the key to be inserted.
 */
int get_left_index(node * parent, pagenum_t pagenum) {

    int left_index = 0;
    while (left_index <= parent->num_keys && 
            parent->pages[left_index] != pagenum)
        left_index++;
	return left_index;
}

/* Inserts a new pointer to a record and its corresponding
 * key into a leaf.
 * Returns the altered leaf.
 */
node * insert_into_leaf( node * leaf, int64_t key, const char * value ) {

    int i, insertion_point;

    insertion_point = 0;
    while (insertion_point < leaf->num_keys && leaf->keys[insertion_point] < key)
        insertion_point++;

    for (i = leaf->num_keys; i > insertion_point; i--) {
        leaf->keys[i] = leaf->keys[i - 1];
        leaf->pointers[i] = leaf->pointers[i - 1];
    }
    leaf->keys[insertion_point] = key;
    leaf->pointers[insertion_point] = value;
    leaf->num_keys++;
	node_to_page(&leaf, true);
	return leaf;
}


/* Inserts a new key and pointer
 * to a new record into a leaf so as to exceed
 * the tree's order, causing the leaf to be split
 * in half.
 */
pagenum_t insert_into_leaf_after_splitting(pagenum_t root, node * leaf, int64_t key, const char * value) {

    node * new_leaf;
    int64_t * temp_keys;
    record * temp_pointers;
    int insertion_index, split, i, j;
	int64_t new_key;

    new_leaf = make_leaf(leaf->table_id);

    temp_keys = (int64_t*)malloc( leaf_order * sizeof(int64_t) );
    if (temp_keys == NULL) {
        perror("Temporary keys array.");
        exit(EXIT_FAILURE);
    }

    temp_pointers = (record*)malloc( leaf_order * sizeof(record) );
    if (temp_pointers == NULL) {
        perror("Temporary pointers array.");
        exit(EXIT_FAILURE);
    }

    insertion_index = 0;
    while (insertion_index < leaf_order - 1 && leaf->keys[insertion_index] < key)
        insertion_index++;

    for (i = 0, j = 0; i < leaf->num_keys; i++, j++) {
        if (j == insertion_index) j++;
        temp_keys[j] = leaf->keys[i];
        temp_pointers[j] = leaf->pointers[i];
    }

    temp_keys[insertion_index] = key;
    temp_pointers[insertion_index] = value;

    leaf->num_keys = 0;

    split = cut(leaf_order - 1);

    for (i = 0; i < split; i++) {
        leaf->pointers[i] = temp_pointers[i];
        leaf->keys[i] = temp_keys[i];
        leaf->num_keys++;
    }

    for (i = split, j = 0; i < leaf_order; i++, j++) {
        new_leaf->pointers[j] = temp_pointers[i];
        new_leaf->keys[j] = temp_keys[i];
        new_leaf->num_keys++;
    }

    free(temp_pointers);
    free(temp_keys);

    new_leaf->pages[leaf_order - 1] = leaf->pages[leaf_order - 1];
    leaf->pages[leaf_order - 1] = new_leaf->pagenum;

    for (i = leaf->num_keys; i < leaf_order - 1; i++)
        leaf->pointers[i] = "";
    for (i = new_leaf->num_keys; i < leaf_order - 1; i++)
        new_leaf->pointers[i] = "";

    new_leaf->parent = leaf->parent;
    new_key = new_leaf->keys[0];
	
    return insert_into_parent(root, leaf, new_key, new_leaf);
}


/* Inserts a new key and pointer to a node
 * into a node into which these can fit
 * without violating the B+ tree properties.
 */
pagenum_t insert_into_node(pagenum_t root, node * n, 
        int left_index, int64_t key, pagenum_t right_pagenum) {
    int i;

    for (i = n->num_keys; i > left_index; i--) {
        n->pages[i + 1] = n->pages[i];
        n->keys[i] = n->keys[i - 1];
    }
    n->pages[left_index + 1] = right_pagenum;
    n->keys[left_index] = key;
    n->num_keys++;
	node_to_page(&n, true);
    return root;
}


/* Inserts a new key and pointer to a node
 * into a node, causing the node's size to exceed
 * the order, and causing the node to split into two.
 */
pagenum_t insert_into_node_after_splitting(pagenum_t root, node * old_node, int left_index, 
        int64_t key, pagenum_t right_pagenum) {

    int i, j, split;
	int64_t k_prime;
    node * new_node, * child = NULL;
    int64_t * temp_keys;
    pagenum_t * temp_pointers;

    /* First create a temporary set of keys and pointers
     * to hold everything in order, including
     * the new key and pointer, inserted in their
     * correct places. 
     * Then create a new node and copy half of the 
     * keys and pointers to the old node and
     * the other half to the new.
     */

    temp_pointers = (pagenum_t*)malloc( (internal_order + 1) * sizeof(node *) );
    if (temp_pointers == NULL) {
        perror("Temporary pointers array for splitting nodes.");
        exit(EXIT_FAILURE);
    }
    temp_keys = (int64_t*)malloc( internal_order * sizeof(int64_t) );
    if (temp_keys == NULL) {
        perror("Temporary keys array for splitting nodes.");
        exit(EXIT_FAILURE);
    }

    for (i = 0, j = 0; i < old_node->num_keys + 1; i++, j++) {
        if (j == left_index + 1) j++;
        temp_pointers[j] = old_node->pages[i];
    }

    for (i = 0, j = 0; i < old_node->num_keys; i++, j++) {
        if (j == left_index) j++;
        temp_keys[j] = old_node->keys[i];
    }

    temp_pointers[left_index + 1] = right_pagenum;
    temp_keys[left_index] = key;

    /* Create the new node and copy
     * half the keys and pointers to the
     * old and half to the new.
     */  
    split = cut(internal_order);
    new_node = make_node(old_node->table_id);
    old_node->num_keys = 0;
    for (i = 0; i < split - 1; i++) {
        old_node->pages[i] = temp_pointers[i];
        old_node->keys[i] = temp_keys[i];
        old_node->num_keys++;
    }
    old_node->pages[i] = temp_pointers[i];
    k_prime = temp_keys[split - 1];
    for (++i, j = 0; i < internal_order; i++, j++) {
        new_node->pages[j] = temp_pointers[i];
        new_node->keys[j] = temp_keys[i];
        new_node->num_keys++;
    }
    new_node->pages[j] = temp_pointers[i];
    free(temp_pointers);
    free(temp_keys);
    new_node->parent = old_node->parent;
    for (i = 0; i <= new_node->num_keys; i++) {
		page_to_node(new_node->table_id, new_node->pages[i], &child);
        child->parent = new_node->pagenum;
		node_to_page(&child, true);
    }
    /* Insert a new key into the parent of the two
     * nodes resulting from the split, with
     * the old node to the left and the new to the right.
     */

    return insert_into_parent(root, old_node, k_prime, new_node);
}



/* Inserts a new node (leaf or internal node) into the B+ tree.
 * Returns the root of the tree after insertion.
 */
pagenum_t insert_into_parent(pagenum_t root, node * left, int64_t key, node * right) {

    int left_index;
    node * parent = NULL;
	pagenum_t right_pagenum;

    page_to_node(left->table_id, left->parent, &parent);

    /* Case: new root. */

    if (parent == NULL)
        return insert_into_new_root(left, key, right);

    /* Case: leaf or node. (Remainder of
     * function body.)  
     */

    /* Find the parent's pointer to the left 
     * node.
     */

    left_index = get_left_index(parent, left->pagenum);
	right_pagenum = right->pagenum;
	node_to_page(&left, true);
	node_to_page(&right, true);

    /* Simple case: the new key fits into the node. 
     */

    if (parent->num_keys < internal_order - 1)
        return insert_into_node(root, parent, left_index, key, right_pagenum);

    /* Harder case:  split a node in order 
     * to preserve the B+ tree properties.
     */

    return insert_into_node_after_splitting(root, parent, left_index, key, right_pagenum);
}


/* Creates a new root for two subtrees
 * and inserts the appropriate key into
 * the new root.
 */
pagenum_t insert_into_new_root(node * left, int64_t key, node * right) {

    node * root = make_node(left->table_id);
	pagenum_t ret = root->pagenum;
    root->keys[0] = key;
    root->pages[0] = left->pagenum;
    root->pages[1] = right->pagenum;
    root->num_keys++;
    root->parent = 0;
    left->parent = root->pagenum;
    right->parent = root->pagenum;
	node_to_page(&left, true);
	node_to_page(&right, true);
	node_to_page(&root, true);
	return ret;
}



/* First insertion:
 * start a new tree.
 */
pagenum_t start_new_tree(int table_id, int64_t key, const char * value) {

    node * root = make_leaf(table_id);
    root->keys[0] = key;
    root->pointers[0] = value;
    root->pages[leaf_order - 1] = 0;
    root->parent = 0;
    root->num_keys++;
	pagenum_t ret = root->pagenum;
	node_to_page(&root, true);
    return ret;
}



/* Master insertion function.
 * Inserts a key and an associated value into
 * the B+ tree, causing the tree to be adjusted
 * however necessary to maintain the B+ tree
 * properties.
 */
pagenum_t insert( int table_id, pagenum_t root, int64_t key, const char* value ) {
    
	node * leaf;

    /* The current implementation ignores
     * duplicates.
     */
	
	leaf = find_leaf(table_id, root, key);
	int ret = find_record(leaf, key);
    if (root && ret != -1){
		free_node(&leaf);
        return -1;
	}

    /* Case: the tree does not exist yet.
     * Start a new tree.
     */

    if (root == 0){
		//leaf is NULL!
        return start_new_tree(table_id, key, value);
	}


    /* Case: the tree already exists.
     * (Rest of function body.)
     */

//    leaf = find_leaf(table_id, root, key);

    /* Case: leaf has room for key and pointer.
     */
	
    if (leaf->num_keys < leaf_order - 1) {
        leaf = insert_into_leaf(leaf, key, value);
        return root;
    }


    /* Case:  leaf must be split.
     */

    return insert_into_leaf_after_splitting(root, leaf, key, value);
}




// DELETION.

/* Utility function for deletion.  Retrieves
 * the index of a node's nearest neighbor (sibling)
 * to the left if one exists.  If not (the node
 * is the leftmost child), returns -1 to signify
 * this special case.
 */
int get_neighbor_index( node * n ) {

    int i;

    /* Return the index of the key to the left
     * of the pointer in the parent pointing
     * to n.  
     * If n is the leftmost child, this means
     * return -1.
     */
	node* parent = NULL;
	page_to_node(n->table_id, n->parent, &parent);
    for (i = 0; i <= parent->num_keys; i++)
        if (parent->pages[i] == n->pagenum){
				free_node(&parent);
				return i - 1;
		}

	free_node(&parent);
    // Error state.
    printf("Search for nonexistent pointer to node in parent.\n");
    printf("Node:  %#lx\n", (unsigned long)n);
    exit(EXIT_FAILURE);
}


node * remove_entry_from_node(node * n, int64_t key, node * pointer) {

    int i, num_pointers;

    // Remove the key and shift other keys accordingly.
    i = 0;
    while (n->keys[i] != key)
        i++;
    for (++i; i < n->num_keys; i++)
        n->keys[i - 1] = n->keys[i];

    // Remove the pointer and shift other pointers accordingly.
    // First determine number of pointers.
    num_pointers = n->is_leaf ? n->num_keys : n->num_keys + 1;
    i = 0;
	if(n->is_leaf){
		while(i != (pagenum_t)pointer) i++;
		for(++i; i<num_pointers; i++){
			n->pointers[i-1] = n->pointers[i];
		}
	}
	else{
		while(n->pages[i] != pointer->pagenum) i++;
		for(++i; i<num_pointers;i ++)
			n->pages[i-1] = n->pages[i];
	}
	
    // One key fewer.
    n->num_keys--;

    // Set the other pointers to NULL for tidiness.
    // A leaf uses the last pointer to point to the next leaf.
    if (n->is_leaf)
        for (i = n->num_keys; i < leaf_order - 1; i++)
            n->pointers[i] = "";
    else
        for (i = n->num_keys + 1; i < internal_order; i++)
            n->pages[i] = 0;
    return n;
}


pagenum_t adjust_root(node * root) {

    node * new_root = NULL;
	pagenum_t ret = 0;

    /* Case: nonempty root.
     * Key and pointer have already been deleted,
     * so nothing to be done.
     */
    if (root->num_keys > 0){
        ret = root->pagenum;
		node_to_page(&root, true);
		return ret;
	}
    /* Case: empty root. 
     */

    // If it has a child, promote 
    // the first (only) child
    // as the new root.

    if (!root->is_leaf) {
        page_to_node(root->table_id, root->pages[0], &new_root);
        new_root->parent = 0;
		ret = new_root->pagenum;
    }

    // If it is a leaf (has no children),
    // then the whole tree is empty.

    else
        new_root = NULL;

	file_free_page(root->table_id, root->pagenum);
	free_node(&root);
	node_to_page(&new_root, true);
    return ret;
}


/* Coalesces a node that has become
 * too small after deletion
 * with a neighboring node that
 * can accept the additional entries
 * without exceeding the maximum.
 */
pagenum_t coalesce_nodes(pagenum_t root, node * n, node * neighbor, int neighbor_index, int64_t k_prime) {

    int i, j, neighbor_insertion_index, n_end;
    node * tmp;

    /* Swap neighbor with node if node is on the
     * extreme left and neighbor is to its right.
     */

    if (neighbor_index == -1) {
        tmp = n;
        n = neighbor;
        neighbor = tmp;
    }

	tmp = NULL;

    /* Starting point in the neighbor for copying
     * keys and pointers from n.
     * Recall that n and neighbor have swapped places
     * in the special case of n being a leftmost child.
     */

    neighbor_insertion_index = neighbor->num_keys;

    /* Case:  nonleaf node.
     * Append k_prime and the following pointer.
     * Append all pointers and keys from the neighbor.
     */

    if (!n->is_leaf) {

        /* Append k_prime.
         */

        neighbor->keys[neighbor_insertion_index] = k_prime;
        neighbor->num_keys++;


        n_end = n->num_keys;

        for (i = neighbor_insertion_index + 1, j = 0; j < n_end; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pages[i] = n->pages[j];
            neighbor->num_keys++;
            n->num_keys--;
        }

        /* The number of pointers is always
         * one more than the number of keys.
         */

        neighbor->pages[i] = n->pages[j];

        /* All children must now point up to the same parent.
         */

        for (i = 0; i < neighbor->num_keys + 1; i++) {
            page_to_node(neighbor->table_id, neighbor->pages[i], &tmp);
            tmp->parent = neighbor->pagenum;
			node_to_page(&tmp, true);
        }
    }

    /* In a leaf, append the keys and pointers of
     * n to the neighbor.
     * Set the neighbor's last pointer to point to
     * what had been n's right neighbor.
     */

    else {
        for (i = neighbor_insertion_index, j = 0; j < n->num_keys; i++, j++) {
            neighbor->keys[i] = n->keys[j];
            neighbor->pointers[i] = n->pointers[j];
            neighbor->num_keys++;
        }
		n->num_keys = 0;
        neighbor->pages[leaf_order - 1] = n->pages[leaf_order - 1];
    }
	
	node_to_page(&neighbor,true);
	node* parent = NULL;
	page_to_node(n->table_id, n->parent, &parent);
    root = delete_entry(root, parent, k_prime, n);
	file_free_page(n->table_id, n->pagenum);
	free_node(&n);
    return root;
}


/* Redistributes entries between two nodes when
 * one has become too small after deletion
 * but its neighbor is too big to append the
 * small node's entries without exceeding the
 * maximum
 */
pagenum_t redistribute_nodes(pagenum_t root, node * n, node * neighbor, int neighbor_index, 
        int64_t k_prime_index, int64_t k_prime) {  

    int i;
    node * tmp = NULL;
	node* parent = NULL;
	page_to_node(n->table_id, n->parent, &parent);

    /* Case: n has a neighbor to the left. 
     * Pull the neighbor's last key-pointer pair over
     * from the neighbor's right end to n's left end.
     */

    if (neighbor_index != -1) {
        if (!n->is_leaf)
            n->pages[n->num_keys + 1] = n->pages[n->num_keys];
        for (i = n->num_keys; i > 0; i--) {
            n->keys[i] = n->keys[i - 1];
            n->pointers[i] = n->pointers[i - 1];
        }
        if (!n->is_leaf) {
            n->pages[0] = neighbor->pages[neighbor->num_keys];
            page_to_node(n->table_id, n->pages[0], &tmp);
            tmp->parent = n->pagenum;
			node_to_page(&tmp, true);
            neighbor->pages[neighbor->num_keys] = 0;
            n->keys[0] = k_prime;
            parent->keys[k_prime_index] = neighbor->keys[neighbor->num_keys - 1];
        }
        else {
            n->pointers[0] = neighbor->pointers[neighbor->num_keys - 1];
            neighbor->pointers[neighbor->num_keys - 1] = NULL;
            n->keys[0] = neighbor->keys[neighbor->num_keys - 1];
			parent->keys[k_prime_index] = n->keys[0];
        }
    }

    /* Case: n is the leftmost child.
     * Take a key-pointer pair from the neighbor to the right.
     * Move the neighbor's leftmost key-pointer pair
     * to n's rightmost position.
     */

    else {  
        if (n->is_leaf) {
            n->keys[n->num_keys] = neighbor->keys[0];
            n->pointers[n->num_keys] = neighbor->pointers[0];
            parent->keys[k_prime_index] = neighbor->keys[1];
        }
        else {
            n->keys[n->num_keys] = k_prime;
            n->pages[n->num_keys + 1] = neighbor->pages[0];
            page_to_node(n->table_id, n->pages[n->num_keys + 1], &tmp);
            tmp->parent = n->pagenum;
			node_to_page(&tmp, true);
            parent->keys[k_prime_index] = neighbor->keys[0];
        }
        for (i = 0; i < neighbor->num_keys - 1; i++) {
            neighbor->keys[i] = neighbor->keys[i + 1];
			if(n->is_leaf)
            	neighbor->pointers[i] = neighbor->pointers[i + 1];
			else
				neighbor->pages[i] = neighbor->pages[i+1];
        }
        if (!n->is_leaf)
            neighbor->pages[i] = neighbor->pages[i + 1];
    }

    /* n now has one more key and one more pointer;
     * the neighbor has one fewer of each.
     */

    n->num_keys++;
    neighbor->num_keys--;
	node_to_page(&n, true);
	node_to_page(&neighbor, true);
	node_to_page(&parent, true);

    return root;
}


/* Deletes an entry from the B+ tree.
 * Removes the record and its key and pointer
 * from the leaf, and then makes all appropriate
 * changes to preserve the B+ tree properties.
 */
pagenum_t delete_entry( pagenum_t root, node * n, int64_t key, void * pointer ) {

    int min_keys;
    node * neighbor = NULL;
    int neighbor_index;
    int64_t k_prime_index, k_prime;
    int capacity;

    // Remove key and pointer from node.

    n = remove_entry_from_node(n, key, (node*)pointer);

    /* Case:  deletion from the root. 
     */

    if (n->pagenum == root) 
        return adjust_root(n);


    /* Case:  deletion from a node below the root.
     * (Rest of function body.)
     */

    /* Determine minimum allowable size of node,
     * to be preserved after deletion.
     */
	
	min_keys = 1;

    /* Case:  node stays at or above minimum.
     * (The simple case.)
     */

    if (n->num_keys >= min_keys){
		node_to_page(&n, true);
		return root;
	}

    /* Case:  node falls below minimum.
     * Either coalescence or redistribution
     * is needed.
     */

    /* Find the appropriate neighbor node with which
     * to coalesce.
     * Also find the key (k_prime) in the parent
     * between the pointer to node n and the pointer
     * to the neighbor.
     */

    neighbor_index = get_neighbor_index( n );
    k_prime_index = neighbor_index == -1 ? 0 : neighbor_index;
	node* parent = NULL;
	page_to_node(n->table_id, n->parent, &parent);
    k_prime = parent->keys[k_prime_index];
    neighbor_index == -1 ?
		page_to_node(parent->table_id, parent->pages[1], &neighbor) : 
        page_to_node(parent->table_id, parent->pages[neighbor_index], &neighbor);

	free_node(&parent);
    capacity = n->is_leaf ? leaf_order : internal_order - 1;

    /* Coalescence. */

    if (neighbor->num_keys + n->num_keys < capacity)
        return coalesce_nodes(root, n, neighbor, neighbor_index, k_prime);

    /* Redistribution. */

    else
        return redistribute_nodes(root, n, neighbor, neighbor_index, k_prime_index, k_prime);
}



/* Master deletion function.
 */
pagenum_t delete_main(int table_id, pagenum_t root, int64_t key) {

    node * key_leaf;
    int key_record_idx;

    key_leaf = find_leaf(table_id, root, key);
    key_record_idx = find_record(key_leaf, key);

    if (key_record_idx != -1 && key_leaf != NULL) {
        root = delete_entry(root, key_leaf, key, (void*)key_record_idx); 
    	return root;
    }
	free_node(&key_leaf);
	return -1;
}
