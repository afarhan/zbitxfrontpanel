/*
	theap : Text heap
	it allocates/frees fixed strings of ascii text
	from a global heap.
	this is useful where strings like list boxes, etc.
	are to be dymaically alloted and removed.

	Do Not change
*/

#ifndef DATASTRUCT_H 
#define DATASTRUCT_H

void theap_init();
const char *theap_dup(const char *string);
void  theap_free(const char *string);
void theap_dump();

struct node{
	void *ptr;
	int id;
	int val;
	struct node *next;  
};
#endif
void list_init();
struct node *node_alloc();
void node_free(struct node *p);
void node_delete_next(struct node *p);
struct node *node_insert_after(struct node *t);
void structures_init();
