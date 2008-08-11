#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdint.h>

#include <libxml/tree.h>
#include <libxml/parser.h>
#include <libxml/xpath.h>
#include <libxml/xpathInternals.h>

#include "automatic.h"
#include "feed_item.h"
#include "downloads.h"
#include "output.h"
#include "utils.h"
#include "web.h"

struct rssNode {
	char *url;
	char *type;
};

typedef struct rssNode rssNode;

void freeNode(rssNode *rss) {
	if(rss) {
		am_free(rss->url);
		rss->url = NULL;
		am_free(rss->type);
		rss->type = NULL;
	}
	am_free(rss);
	rss = NULL;
}

int getNodeText(xmlNodePtr child, char **dest) {
	xmlChar * textNode;
	int result = 0;

	textNode = xmlNodeGetContent(child);
	*dest = am_strdup((char*)textNode);
	xmlFree(textNode);
	if(*dest)
		result = 1;
	return result;
}

rssNode* getNodeAttributes(xmlNodePtr child) {
	rssNode *tmp = am_malloc(sizeof(rssNode));
	xmlAttrPtr attr = child->properties;
	while(attr) {
		if((strcmp((char*)attr->name, "url") == 0)) {
			getNodeText(attr->children, &tmp->url);
		} else if((strcmp((char*)attr->name, "content") == 0) ||
					 (strcmp((char*)attr->name, "type") == 0)) {
			getNodeText(attr->children, &tmp->type);
		}
		attr = attr->next;
	}
	return tmp;
}

static void extract_feed_items(xmlNodeSetPtr nodes) {
	xmlNodePtr cur, child;
	uint32_t size, i;
	feed_item item;
	uint8_t name_set, url_set, is_torrent_feed = 0;
	rssNode *enclosure;
	size = (nodes) ? nodes->nodeNr : 0;

/*	if(size > am_get_bucket_size()) {
		dbg_printf(P_INFO2, "bucketsize_changed: %d", size);
		am_set_bucket_size(size);
	}
*/

 	dbg_printf(P_INFO, "%d items in XML", size);
	for(i = 0; i < size; ++i) {
		assert(nodes->nodeTab[i]);
		if(nodes->nodeTab[i]->type == XML_ELEMENT_NODE) {
			cur = nodes->nodeTab[i];
			if(cur->children) {
				child = cur->children;
				url_set = 0;
				name_set = 0;
				item = newFeedItem();
				while(child) {
					if((strcmp((char*)child->name, "title") == 0)) {
						name_set = getNodeText(child->children, &item->name);
					} else if((strcmp((char*)child->name, "link" ) == 0)) {
						if(url_set == 0) {			/* if "enclosure" was scanned before "link", use the former */
							url_set = getNodeText(child->children, &item->url);
						}
					} else if((strcmp((char*)child->name, "enclosure" ) == 0)) {
						enclosure = getNodeAttributes(child);
						if((strcmp(enclosure->type, "application/x-bittorrent") == 0)) {
							if(enclosure->url) {
								if(item->url) {
									am_free(item->url);
								}
								item->url = am_strdup(enclosure->url);
								url_set = 1;
								is_torrent_feed = 1;
								freeNode(enclosure);
							}
						}
					}
					child = child->next;
				}
				if(is_torrent_feed == 0) {
					dbg_printf(P_MSG, "Is this really a torrent feed?");
				}
				if(name_set && url_set) {
					applyFilters(item);
				}
				freeFeedItem(item);
				child = cur = NULL;
			}
		} else {
			cur = nodes->nodeTab[i];
			dbg_printf(P_ERROR, "Unknown node \"%s\": type %d", cur->name, cur->type);
		}
	}
	dbg_printf(P_INFO2, "== Done extracting RSS items ==");
}

int parse_xmldata(const char* buffer, int size) {
	xmlDocPtr doc;
	xmlXPathContextPtr xpathCtx;
	xmlXPathObjectPtr xpathObj;
	xmlNodeSetPtr ttlNode;

	static int ttl = 0;
	uint32_t item_count = 0;
	const xmlChar* ttlExpr = (xmlChar*)"//channel/ttl";
	const xmlChar* itemExpr = (xmlChar*)"//item";
	LIBXML_TEST_VERSION

	/* Init libxml */
	xmlInitParser();

	assert(buffer);

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

	/* check for time-to-live element in RSS feed */
	if(ttl == 0) {
		xpathObj = xmlXPathEvalExpression(ttlExpr, xpathCtx);
		if(xpathObj != NULL) {
			ttlNode = xpathObj->nodesetval;

			if(ttlNode->nodeNr == 1 && ttlNode->nodeTab[0]->type == XML_ELEMENT_NODE) {
				ttl = atoi((char*)xmlNodeGetContent(ttlNode->nodeTab[0]->children));

				/* user-specified interval is shorter than that requested by the RSS feed */
				if(ttl > am_get_interval()) {
					am_set_interval(ttl);
				}
			}
			xmlXPathFreeObject(xpathObj);
		} else {
			dbg_printf(P_ERROR, "Error: unable to evaluate TTL XPath expression");
		}
	}

	/* Extract RSS "items" from feed */
	xpathObj = xmlXPathEvalExpression(itemExpr, xpathCtx);
	if(xpathObj == NULL) {
		dbg_printf(P_ERROR, "Error: unable to evaluate XPath expression \"%s\"", itemExpr);
		xmlXPathFreeContext(xpathCtx);
		xmlFreeDoc(doc);
		xmlCleanupParser();
		return -1;
	}

	item_count = xpathObj->nodesetval ? xpathObj->nodesetval->nodeNr : 0;
	extract_feed_items(xpathObj->nodesetval);

	/* Cleanup */
	xmlXPathFreeObject(xpathObj);
	xmlXPathFreeContext(xpathCtx);
	xmlFreeDoc(doc);
	/* Shutdown libxml */
	xmlCleanupParser();
	return item_count;
}

