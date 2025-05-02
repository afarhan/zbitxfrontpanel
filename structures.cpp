#include <Arduino.h>
#include <stdio.h> 
#include <string.h>
#include "structures.h"

/* we define a heap with a very limited usage: 
	just for text. as a result, all the ununsed parts
	are marked as 0xff character, not used anywhere 
	in regular ascii text
*/

#define MAX_THEAP  10000 
char my_theap[MAX_THEAP+1];

#define MAX_LINKS 100 
struct node linkblock[MAX_LINKS];
struct node *free_list = NULL;

void list_init(){
	free_list = linkblock;

	struct node *prev = linkblock;
	for (int i = 1; i < MAX_LINKS; i++){
		struct node *pl = linkblock + i;
		pl->id = i;
		prev->next = pl;
		prev = pl;	
	}
	//last in the free_list points to null				
}

struct node *node_alloc(){
	if (!free_list)
		return NULL;
	struct node *z = free_list;
	free_list = z->next;
	z->next = NULL;
	//Serial.printf("alloted node %d\n", z->id);
	return z;
}

void node_free(struct node *p){
	p->next = free_list;
	Serial.printf("freed node %d\n", p->id);
	free_list = p;
}

void node_delete_next(struct node *p){
	struct node *z = p->next;
	if (!z)
		return;
	p->next = z->next;

	node_free(z);
}

struct node *node_insert_after(struct node *t){
	if (!free_list)
		return NULL;
	//pluck on off the free_list
	struct node *p = free_list;
	free_list = p->next;

	p->next = t->next;
	t->next = p;
	return p;
}

void theap_init(){
	char *p = my_theap;
	for (int i = 0; i < MAX_THEAP; i++)
		*p++ = 0xff;		
	my_theap[MAX_THEAP] = 0;	
}

/* Text Heap */

//search for a run of -1 chars 
char *theap_alloc(int size){
	char *freestart = NULL;

	for (char *p = my_theap; p < my_theap + MAX_THEAP; p++){
		if (*p == 0xff){
			if (!freestart){
				freestart = p;
			}
			else if (p - freestart == size)
				break;
		}
		else
			freestart = NULL;
	}
	
	if (freestart){
		for (char *p = freestart; p < freestart + size; p++)
			*p = 0;
	}
	return freestart;
}

const char *theap_dup(const char *string){
	char *p = theap_alloc(strlen(string));
	if (p)
		strcpy(p, string);
	return p;
}

void  theap_free(const char *string){
	char * p = (char *)string;
	//check if we are inside the heap
	int size = strlen(p);
	if (p < my_theap || my_theap + MAX_THEAP < p + size+1)
		return; 
	for (int i = 0; i < size; i++)
		*p++ = 0xff;
	*p = -1;  //the last zero is also released
}

void theap_dump(){
	for (int i = 0; i < MAX_THEAP; i++)
		if (my_theap[i] == 0)
			Serial.print('~');
		else if (my_theap[i] == 0xff)
			Serial.print('*');
		else
			Serial.print((char )my_theap[i]);
	Serial.print('\n');
}

void structures_init(){
	theap_init();
	list_init();
}

/* 

void theap_dump(int line){
	printf("%d:", line);
	for (int i = 0; i < MAX_THEAP; i++)
	if (my_theap[i] == 0)
		putchar('~');
	else
		putchar(my_theap[i]);
	putchar('\n');
}

void theap_test(){
	theap_init();
	theap_dump(__LINE__);
	const char *p1 = theap_dup((char *)"hello, how is this?");
	theap_dump(__LINE__);
	const char *p2 = theap_dup((char *)"A really really long string");
	theap_dump(__LINE__);
	theap_free(p1);
	theap_dump(__LINE__);
	const char *p3 = theap_dup((char *)"Hi, short");
	theap_dump(__LINE__);
	theap_free(p2);
	theap_free(p3);
	theap_dump(__LINE__);
}

void list_dump(struct node *p){
	printf("dumping list ");
	while(p){
		printf("%d->", p->id);
		p = p->next;
	}	
	putchar('\n');	
}

int main(int argc, char **argv){
	theap_test();
}

void list_test(){

	list_init();
	list_dump(free_list);
	
	printf("allocting one node\n");
	struct node *l1 = node_alloc();
	list_dump(free_list);
	list_dump(l1);

	//take another
	printf("allocating a new list\n");
	struct node *l2 = node_alloc();
	list_dump(free_list);
	list_dump(l1);
	list_dump(l2);

	//insert two new nodes after l1
	printf("inserting two new nodes on l1\n");
	node_insert_after(l1);
	node_insert_after(l1);

	list_dump(free_list);
	list_dump(l1);
	list_dump(l2);

	printf("removing an element from l1\n");
	//take an element out of the first list
	node_delete_next(l1);
	list_dump(free_list);
	list_dump(l1);
	list_dump(l2);

	printf("freeing the allocated node\n");
	node_free(l1);
	node_free(l2);
	list_dump(free_list);
	
}

int  main(int argc, char **argv){
	list_test();
}
*/
