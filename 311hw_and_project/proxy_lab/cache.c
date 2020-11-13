#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "csapp.h"
#include "cache.h"

/**
 * popback: pop the last item from the list
 * */
static void popback(CacheList *list)
{
  if (list->last)
  {
    //pop the last item
    list->size -= list->last->size;
    CachedItem *last = list->last;
    list->last = last->prev;
    //if the last
    if (list->last)
    {
      list->last->next = NULL;
    }
    else{
      list->first = NULL;
    }
    //free space
    free(last->headers);
    free(last->item_p);
    free(last->url);
    free(last);
  }
}

/**
 * prepend: insert the item into the front of the list
 * */
static void prepend(CacheList *list, CachedItem *citem)
{
  citem->next = list->first;
  list->first = citem;
  if (!list->last)
  {
    list->last = citem;
  }
  list->size += citem->size;
  //if too much cache stored, pop last item
  while (list->size > MAX_CACHE_SIZE)
  {
    popback(list);
  }
}

/* cache_init initializes the input cache linked list. */
void cache_init(CacheList *list)
{
  //initialize
  list->size = 0;
  list->first = NULL;
  list->last = NULL;
}

/* cache_URL adds a new cached item to the linked list. It takes the
 * URL being cached, a link to the content, the size of the content, and
 * the linked list being used. It creates a struct holding the metadata
 * and adds it at the beginning of the linked list.
 */
void cache_URL(const char *URL, const char *headers, void *item, size_t size, CacheList *list)
{
  // check size
  if (size > MAX_OBJECT_SIZE)
  {
    return;
  }
  //allocate space and save in cache
  CachedItem *citem = (CachedItem *)Malloc(sizeof(CachedItem));
  citem->url = strdup(URL);
  citem->headers = strdup(headers);
  citem->item_p = item;
  citem->next = NULL;
  citem->prev = NULL;
  citem->size = size;

  memcpy(citem->item_p, item, size);
  prepend(list, citem);
}

/* find iterates through the linked list and returns a pointer to the
 * struct associated with the requested URL. If the requested URL is
 * not cached, it returns null.
 */
CachedItem *find(const char *URL, CacheList *list)
{
  CachedItem *p = list->first;
  //find the url in the list, null if not cached
  while (p && strcmp(p->url, URL) != 0)
  {
    p = p->next;
  }
  return p;
}

/* frees the memory used to store each cached object, and frees the struct
 * used to store its metadata. */
void cache_destruct(CacheList *list)
{
  CachedItem *current;
  CachedItem *next;
  //destroy cache free memory
  current = list->first;
  while (current)
  {
    next = current->next;
    free(current->url);
    free(current->headers);
    free(current->item_p);
    free(current);
    current = next;
  }
}
