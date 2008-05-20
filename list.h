#ifndef LIST_H__
#define LIST_H__


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
int hasURL(const char *url, NODE *head);

/* list modifiers */
void addRSSItem(rss_item elem, NODE **head);
void freeList(NODE **head);
void deleteHead(NODE **ptr_to_head);
rss_item newRSSItem();
int add_to_bucket(rss_item elem, NODE **b, int use_size_limit);
void freeItem(NODE *item);

#endif
