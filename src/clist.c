/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu May 24 17:25:37 2007 texane
** Last update Thu Nov 01 08:29:53 2007 texane
*/



#include <stdlib.h>
#include "clist.h"



void clist_init(clist_t* list)
{
  list->head = NULL;
  list->tail = NULL;
  list->count = 0;
}


void clist_release(clist_t* list,
		   void (*fn)(clist_node_t*, void*),
		   void* data)
{
  clist_flush(list, fn, data);
}


void clist_flush(clist_t* list,
		 void (*fn)(clist_node_t*, void*),
		 void* data)
{
  clist_node_t* prev;
  clist_node_t* pos;

  pos = clist_head(list);
  while (pos != NULL)
    {
      prev = pos;
      pos = clist_node_next(pos);
      if (fn != NULL)
	fn(prev, data);
      free(prev);
    }

  list->tail = NULL;
  list->head = NULL;
  list->count = 0;
}


void clist_push_front(clist_t* list,
		      clist_node_t* node)
{
  node->next = NULL;
  node->prev = NULL;

  if (list->head != NULL)
    {
      list->head->prev = node;
      node->next = list->head;
    }
  else
    {
      list->tail = node;
    }
  list->head = node;
  ++list->count;
}


void clist_push_back(clist_t* list,
		     clist_node_t* node)
{
  node->next = NULL;
  node->prev = NULL;

  if (list->tail != NULL)
    {
      list->tail->next = node;
      node->prev = list->tail;
    }
  else
    {
      list->head = node;
    }

  list->tail = node;

  ++list->count;
}


void clist_insert_before(clist_t* list,
			 clist_node_t* pos,
			 clist_node_t* node)
{
  node->prev = NULL;
  node->next = NULL;

  if (pos == NULL)
    {
      clist_push_back(list, node);
    }
  else if (pos == clist_head(list))
    {
      clist_push_front(list, node);
    }
  else
    {
      node->next = pos;
      node->prev = pos->prev;
      pos->prev->next = node;
      pos->prev = node;
      ++list->count;
    }
}


void clist_insert_after(clist_t* list,
			clist_node_t* pos,
			clist_node_t* node)
{
  node->next = NULL;
  node->prev = NULL;

  if (pos == NULL)
    {
      clist_push_front(list, node);
    }
  else if (pos == clist_tail(list))
    {
      clist_push_back(list, node);
    }
  else
    {
      pos->next->prev = node;
      node->next = pos->next;
      pos->next = node;
      node->prev = pos;
      ++list->count;
    }
}


int clist_pop_front(clist_t* list,
		    void (*fn)(clist_node_t*, void*),
		    void* data)
{
  clist_node_t* pos;

  /* error */
  if (list->count == 0)
    return -1;

  /* unlink the node */
  pos = list->head;
  if (pos->next != NULL)
    pos->next->prev = NULL;
  else
    list->tail = NULL;

  list->head = pos->next;
  --list->count;

  /* release the node */
  if (fn != NULL)
    fn(pos, data);

  return 0;
}


int clist_pop_back(clist_t* list,
		   void (*fn)(clist_node_t*, void*),
		   void* data)
{
  clist_node_t* pos;

  /* error */
  if (list->count == 0)
    return -1;

  /* unlink */
  pos = list->tail;
  if (pos->prev != NULL)
    pos->prev->next = NULL;
  else
    list->head = NULL;

  list->tail = list->tail->prev;
  --list->count;

  /* release the node */
  if (fn != NULL)
    fn(pos, data);

  return 0;
}


void clist_node_init(clist_node_t* node,
		     void* data)
{
  node->prev = NULL;
  node->next = NULL;
  node->data = data;
}


unsigned int clist_count(const clist_t* list)
{
  return list->count;
}


void clist_affect(clist_t* a,
		  clist_t* b)
{
  a->head = b->head;
  a->tail = b->tail;
  a->count = b->count;
  b->head = NULL;
  b->tail = NULL;
  b->count = 0;
}


void clist_split(clist_t* to,
		 clist_t* from,
		 clist_node_t* pos)
{
  clist_node_t* i;

  if (pos->prev == NULL)
    return ;

  to->head = from->head;
  to->tail = pos->prev;
  to->tail->next = NULL;
  to->count = 0;
  for (i = clist_head(to); i != NULL; i = clist_node_next(i))
    ++to->count;

  from->head = pos;
  pos->prev = NULL;
  from->count = 0;
  for (i = clist_head(from); i != NULL; pos = clist_node_next(pos))
    ++from->count;
}


void clist_append(clist_t* to,
		  clist_t* from)
{
  if (to->tail != NULL)
    {
      to->tail->next = from->head;
      from->head->prev = to->tail;
      to->count += from->count;
    }
  else
    {
      clist_affect(to, from);
    }
}


int clist_remove(clist_t* list,
		 clist_node_t* node,
		 void (*fn)(clist_node_t*, void*),
		 void* data)
{
  /* error */
  if (list->count == 0)
    return -1;
    
  /* unlink the node */
  if (list->count == 1)
    {
      list->head = NULL;
      list->tail = NULL;
    }
  else if (list->head == node)
    {
      list->head = node->next;
      node->next->prev = NULL;
    }
  else if (list->tail == node)
    {
      list->tail = node->prev;
      node->prev->next = NULL;
    }
  else
    {
      node->prev->next = node->next;
      node->next->prev = node->prev;
    }

  --list->count;

  /* release the node */
  if (fn != NULL)
    fn(node, data);

  return 0;
}


void clist_foreach(clist_t* list,
		   int (*fn)(clist_node_t*, void*),
		   void* data)
{
  clist_node_t* tmp;
  clist_node_t* pos;

  if (fn == NULL)
    return ;

  pos = clist_head(list);
  while (pos != NULL)
    {
      tmp = pos;
      pos = clist_node_next(pos);
      if (fn(tmp, data) == -1)
	return ;
    }
}


int clist_find(const clist_t* list,
	       int (*fn)(const clist_node_t*, void*),
	       void* data,
	       clist_node_t** res)
{
  clist_node_t* pos;

  *res = NULL;

  for (pos = clist_head((clist_t*)list);
       pos != NULL;
       pos = clist_node_next(pos))
    if (fn(pos, data) == 0)
      {
	*res = pos;
	return 0;
      }

  return -1;
}


void clist_unlink_node(clist_t* list,
		       clist_node_t* node)
{
  /* unlink the node */
  if (list->count == 1)
    {
      list->head = NULL;
      list->tail = NULL;
    }
  else if (list->head == node)
    {
      list->head = node->next;
      node->next->prev = NULL;
    }
  else if (list->tail == node)
    {
      list->tail = node->prev;
      node->prev->next = NULL;
    }
  else
    {
      node->prev->next = node->next;
      node->next->prev = node->prev;
    }

  --list->count;

  node->next = NULL;
  node->prev = NULL;
}


static void clist_swap_nodes(clist_t* list,
			     clist_node_t* node_a,
			     clist_node_t* node_b)
{
  clist_node_t* pos_a;
  clist_node_t* pos_b;

  /* neighbors */
  if (node_a->next == node_b)
    {
      clist_unlink_node(list, node_a);
      clist_insert_after(list, node_b, node_a);
      return ;
    }
  else if (node_b->next == node_a)
    {
      clist_unlink_node(list, node_b);
      clist_insert_after(list, node_a, node_b);
      return ;
    }

  /* get posistions */
  pos_a = clist_node_next(node_b);
  pos_b = clist_node_next(node_a);

  /* unlink nodes */
  clist_unlink_node(list, node_a);
  clist_unlink_node(list, node_b);

  /* insert nodes */
  clist_insert_before(list, pos_a, node_a);
  clist_insert_before(list, pos_b, node_b);
}


void clist_sort(clist_t* list,
		int (*cmp)(const clist_node_t*, const clist_node_t*, void*),
		void* data)
{
  clist_node_t* i;
  clist_node_t* j;
  clist_node_t* k;
  clist_node_t* next;

  i = clist_head(list);
  while (i != NULL)
    {
      /* get valuest node */
      k = i;
      for (j = clist_node_next(i);
	   j != NULL;
	   j = clist_node_next(j))
	{
	  /* i > j */
	  if (cmp(k, j, data) > 0)
	    k = j;
	}

      /* swap nodes */
      if (k != i)
	{
	  /* save next before swapping */
	  if (k != clist_node_next(i))
	    next = clist_node_next(i);
	  else
	    next = i;

	  clist_swap_nodes(list, i, k);
	}
      else
	{
	  next = clist_node_next(i);
	}

      /* next node */
      i = next;
    }
}


clist_node_t* clist_head(clist_t* list)
{
  return list->head;
}


clist_node_t* clist_tail(clist_t* list)
{
  return list->tail;
}


clist_node_t* clist_at(clist_t* list,
		       unsigned int i)
{
  clist_node_t* pos;

  pos = NULL;

  if (i < clist_count(list))
    {
      pos = clist_head(list);
      while (i)
	{
	  pos = clist_node_next(pos);
	  --i;
	}
    }

  return pos;
}


clist_node_t* clist_node_next(clist_node_t* node)
{
  return node->next;
}


clist_node_t* clist_node_prev(clist_node_t* node)
{
  return node->prev;
}


void* clist_node_data(clist_node_t* node)
{
  return node->data;
}


const void* clist_node_const_data(const clist_node_t* node)
{
  return node->data;
}


int clist_node_new(clist_node_t** node,
		   void* data)
{
  *node = malloc(sizeof(clist_node_t));
  if (*node == NULL)
    return -1;

  clist_node_init(*node, data);

  return 0;
}


void clist_node_delete(clist_node_t* node,
		       void (*fn)(clist_node_t*, void*),
		       void* data)
{
  if (fn != NULL)
    fn(node, data);
  free(node);
}
