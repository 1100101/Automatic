/*
 * list_test.c
 *
 *  Created on: Oct 20, 2008
 *      Author: aurich
 */

#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <time.h>

#include "list.h"
#include "utils.h"
#include "output.h"

#define LIST_SIZE 100

static char *rand_str(uint8_t size) {
   static const char text[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
   int i, len = rand() % (size - 1);
   char *dst = am_malloc(len + 1);
   for ( i = 0; i < len; ++i ) {
     dst[i] = text[rand() % (sizeof text - 1)];
   }
   dst[i] = '\0';
   return dst;
}

static int testIntegerList(void){
	uint32_t i;

	int *number;
	simple_list list = NULL;

	assert(listCount(list) == 0);
	for(i = 0; i < LIST_SIZE; i++) {
		number = (int*)am_malloc(sizeof(int));
		*number = i;
		addItem(number, &list);
	}
	assert(listCount(list) == LIST_SIZE);
	assert(*(int*)list->data == *number);
	reverseList(&list);
	assert(*(int*)list->data == 0);
	assert(listCount(list) == LIST_SIZE);
	removeFirst(&list, NULL);
	assert(*(int*)list->data == 1);
	assert(listCount(list) == LIST_SIZE - 1);
	removeLast(list, NULL);
	assert(*(int*)list->data == 1);
	assert(listCount(list) == LIST_SIZE - 2);
	freeList(&list, NULL);
	assert(listCount(list) == 0);
	assert(list == NULL);
	return 0;
}

static int testStringList(void){
	uint32_t i;
	char *str = NULL;
	simple_list list = NULL;

	/* these should do nothing */
	srand(time(0));
	freeList(&list, NULL);
	removeFirst(&list, NULL);
	removeLast(list, NULL);

	for(i = 0; i < LIST_SIZE; i++) {
		str = rand_str(200);
		addItem(str, &list);
	}
	assert(listCount(list) == LIST_SIZE);
	reverseList(&list);
	assert(listCount(list) == LIST_SIZE);
	removeFirst(&list, NULL);
	assert(listCount(list) == LIST_SIZE - 1);
	removeFirst(&list, NULL);
	assert(listCount(list) == LIST_SIZE - 2);
	reverseList(&list);
	assert(strcmp(str, (char*)list->data) == 0);
	removeLast(list, NULL);
	assert(listCount(list) == LIST_SIZE - 3);
	removeLast(list, NULL);
	assert(listCount(list) == LIST_SIZE - 4);
	freeList(&list, NULL);
	assert(listCount(list) == 0);
	return 0;
}

int main (int argc, char **argv) {
	int ret = 0;
	ret += testIntegerList();
	ret += testStringList();
	return ret;
}
