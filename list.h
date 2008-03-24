#ifndef LIST_H__
#define LIST_H__
/* #define MEMWATCH 1 */
#include "memwatch.h"

typedef struct NODE NODE;
typedef struct list_elem list_elem;
typedef NODE* linked_list;

struct list_elem {
	char *name;
	char *url;
};

struct NODE {
   NODE *pNext;
   list_elem elem;
};

void addItem(list_elem elem, NODE **head);
list_elem findItem(char *str, NODE *head);
void freeList(NODE **head);
int hasItem(unsigned int addr, NODE *head);
void deleteHead(NODE **ptr_to_head);
unsigned int listCount(NODE *head);


#endif
