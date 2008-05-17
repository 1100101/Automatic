#ifndef LIST_H__
#define LIST_H__
#include "memwatch.h"


typedef struct NODE NODE;
typedef struct rss_item rss_item;
typedef NODE* linked_list;

struct rss_item {
	char *name;
	char *url;
};

struct NODE {
   NODE *pNext;
   rss_item item;
};

/* read-only functions */
unsigned int listCount(NODE *head);
int hasURL(char *url, NODE *head);
int hasItem(unsigned int addr, NODE *head);

/* list modifiers */
void addItem(rss_item elem, NODE **head);
rss_item findItem(char *str, NODE *head);
void freeList(NODE **head);
void deleteHead(NODE **ptr_to_head);

#endif
