#ifndef LIST_H__
#define LIST_H__

#define MAX_LEN 100

typedef struct NODE NODE;
typedef struct list_elem list_elem;
typedef NODE *linked_list;

struct list_elem {
	/* char name[MAX_LEN]; */
	char *name;
	char *url;
};

struct NODE {
   NODE *pNext;
   list_elem elem;
};

void addItem(list_elem elem, NODE **head);
void deleteItem(unsigned int addr, NODE **head);
list_elem findItem(unsigned int addr, NODE *head);
int hasItem(unsigned int addr, NODE *head);
unsigned int listCount(NODE *head);
void freeList(NODE **head);

#endif
