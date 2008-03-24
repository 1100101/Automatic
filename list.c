#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "list.h"

void insertNode(NODE **ptr_to_head, NODE *newnode) {
	NODE /* *p, */ *prev;

	/* link new element into existing list */
	prev = NULL;
/*	p = *ptr_to_head;
	assert(newnode != NULL);
	while (p != NULL) {						traverse to the last element
		prev = p;
		p = p->pNext;
	}*/
	if (prev == NULL) {						/* insertion at head of list */
		newnode->pNext = *ptr_to_head; 		/* append old list (if any) to new node */
		*ptr_to_head = newnode;				/* new item becomes head of list */
/* 		newnode->pNext = NULL; */
	} else {								/* insert between prev and p */
		prev->pNext = newnode;
		newnode->pNext = NULL;
	}
}

NODE* findNode(char *str, NODE *head) {
	NODE *p = head;

	while (p != NULL) {
		if(strcmp(p->elem.url, str) == 0)
			return p;
		p = p->pNext;
	}
	return NULL;

}

void deleteNode(NODE **ptr_to_head, NODE *pNode) {
	NODE *p, *prev = NULL;

	for (p = *ptr_to_head; p != NULL; prev = p, p = p->pNext) {
		if (p == pNode) {
			if (prev == NULL) {				/* deletion at head of list */
				*ptr_to_head = p->pNext;
			} else {
				prev->pNext = p->pNext;		/* make predecessor point to successor */
			}
			free(p);								/* release memory */
			return;
		}
	}
}

/* public functions */

void freeList(NODE **ptr_to_head) {
	NODE *p = *ptr_to_head;

	while (*ptr_to_head != NULL) {
		p = (*ptr_to_head)->pNext;
		/* release element and check pointer after "free" */
		free(*ptr_to_head);
		/* deleteNode(ptr_to_head, *ptr_to_head); */
		*ptr_to_head = p;
	}
	return;
}

void addItem(list_elem elem, NODE **head) {
	NODE *pNode = (NODE*)malloc(sizeof(NODE));
	if(pNode == NULL)
		perror("malloc failed");
	pNode->elem = elem;
	pNode->pNext = NULL;
	insertNode(head, pNode);
}

int hasURL(char *url, NODE *head) {
	if(findNode(url, head)) {
		return 1;
	} else {
		return 0;
	}
}

void deleteHead(NODE **ptr_to_head) {
 	deleteNode(ptr_to_head, *ptr_to_head);
}

unsigned int listCount(NODE *head) {
	int c = 0;
	NODE *current = head;
	while (current != NULL) {
		++c;
		current = current->pNext;
	}
	return c;
}

/*

int main() {
	NODE *myList = NULL;
	addAddress(12345, &myList);
	addAddress(678910, &myList);
	addAddress(42938423, &myList);
	printf("count: %i\n", countList(myList));
	printf("findNode(12345): %i\n", hasAddress(12345, myList));
	printf("findNode(32343): %i\n", hasAddress(32343, myList));
	deleteAddress(678910, &myList);
	printf("findNode(678910): %i\n", hasAddress(678910, myList));
	printf("count: %i\n", countList(myList));
	deleteAllNodes(&myList);
	printf("count: %i\n", countList(myList));
	return 0;
}

*/
