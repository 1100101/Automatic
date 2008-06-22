#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "automatic.h"
#include "list.h"
#include "output.h"

static void extract_feed_items(xmlNodeSetPtr nodes, NODE **rss_items) {
	xmlNodePtr cur, child;
	xmlChar *textNode;
	int size, i, len;
	rss_item item;
	uint8_t name_set, url_set;
	static int bucketsize_changed = 0;
	size = (nodes) ? nodes->nodeNr : 0;

	if(!bucketsize_changed && size > 0 && size < 256) {
		dbg_printf(P_INFO2, "bucketsize_changed: %d", size);
 		am_set_bucket_size(size);
		bucketsize_changed = 1;
	}

 	dbg_printf(P_INFO, "%d items in XML", size);
	for(i = 0; i < size; ++i) {
		assert(nodes->nodeTab[i]);
		if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
			cur = nodes->nodeTab[i];
			if(cur->children) {
				child = cur->children;
				url_set = 0;
				name_set = 0;
				item = newRSSItem();
				while(child) {
					if((strcmp((char*)child->name, "title") == 0) ||
						(strcmp((char*)child->name, "link" ) == 0)) {
						textNode = xmlNodeGetContent(child->children);
						if(textNode) {
							dbg_printf(P_INFO2, "Found text node '%s'", textNode);
							len = strlen((char*)textNode) + 1;
							if(strcmp((char*)child->name, "title") == 0) {
								item->name = am_malloc(len);
								if(item->name) {
									strncpy(item->name, (char*)textNode, len);
									name_set = 1;
								}
							} else if(strcmp((char*)child->name, "link") == 0) {
								item->url = am_malloc(len);
								if(item->url) {
									strncpy(item->url, (char*)textNode, len);
									url_set = 1;
								}
							}
							xmlFree(textNode);
						}
					}
					if(name_set && url_set) {
						break;
					}
					child = child->next;
				}
				if(name_set && url_set) {
					addItem(item, rss_items);
				} else {
					freeRSSItem(item);
				}
				child = cur = NULL;
			}
		} else {
			cur = nodes->nodeTab[i];
			dbg_printf(P_ERROR, "Unknown node \"%s\": type %d", cur->name, cur->type);
		}
	}
}

int parse_xmldata(const char* buffer, int size, NODE **rss_items) {
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;

	const xmlChar* xpathExpr = (xmlChar*)"//item";
	LIBXML_TEST_VERSION

	/* Init libxml */
	xmlInitParser();

	assert(buffer);
	assert(xpathExpr);

	/* Load XML document */
	doc = xmlParseMemory(buffer, size);
	if (doc == NULL) {
		dbg_printf(P_ERROR, "Error: unable to parse buffer");
		return(-1);
	}

	/* Create xpath evaluation context */
	xpathCtx = xmlXPathNewContext(doc);
	if(xpathCtx == NULL) {
		dbg_printf(P_ERROR, "Error: unable to create new XPath context");
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return -1;
	}

	/* Evaluate xpath expression */
	xpathObj = xmlXPathEvalExpression(xpathExpr, xpathCtx);
	if(xpathObj == NULL) {
		dbg_printf(P_ERROR, "Error: unable to evaluate XPath expression \"%s\"", xpathExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return -1;
	}
	extract_feed_items(xpathObj->nodesetval, rss_items);
	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	/* Shutdown libxml */
	xmlCleanupParser();
	return 0;
}
