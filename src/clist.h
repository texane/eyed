/*
** Made by texane <texane@gmail.com>
** 
** Started on  Thu May 24 17:25:07 2007 texane
** Last update Thu Nov 01 08:30:04 2007 texane
*/


#ifndef CLIST_H_INCLUDED
# define CLIST_H_INCLUDED


typedef struct clist_node
{
  struct clist_node* prev;
  struct clist_node* next;
  void* data;
} clist_node_t;

typedef struct clist
{
  clist_node_t* head;
  clist_node_t* tail;
  unsigned int count;
} clist_t;


void clist_init(clist_t*);
void clist_release(clist_t*, void (*)(clist_node_t*, void*), void*);
void clist_flush(clist_t*, void (*)(clist_node_t*, void*), void*);
void clist_push_front(clist_t*, clist_node_t*);
void clist_push_back(clist_t*, clist_node_t*);
void clist_insert_before(clist_t*, clist_node_t*, clist_node_t*);
void clist_insert_after(clist_t*, clist_node_t*, clist_node_t*);
int clist_pop_front(clist_t*, void (*)(clist_node_t*, void*), void*);
int clist_pop_back(clist_t*, void (*)(clist_node_t*, void*), void*);
int clist_remove(clist_t*, clist_node_t*, void (*)(clist_node_t*, void*), void*);
void clist_node_init(clist_node_t*, void*);
unsigned int clist_count(const clist_t*);
void clist_affect(clist_t*, clist_t*);
void clist_split(clist_t*, clist_t*, clist_node_t*);
void clist_append(clist_t*, clist_t*);
void clist_foreach(clist_t*, int (*)(clist_node_t*, void*), void*);
int clist_find(const clist_t*, int (*)(const clist_node_t*, void*), void*, clist_node_t**);
void clist_sort(clist_t*, int (*)(const clist_node_t*, const clist_node_t*, void*), void*);
void clist_unlink_node(clist_t*, clist_node_t*);

/* accessor */
clist_node_t* clist_head(clist_t*);
clist_node_t* clist_tail(clist_t*);
clist_node_t* clist_at(clist_t*, unsigned int);
clist_node_t* clist_node_next(clist_node_t*);
clist_node_t* clist_node_prev(clist_node_t*);
void* clist_node_data(clist_node_t*);
const void* clist_node_const_data(const clist_node_t*);
int clist_node_new(clist_node_t**, void*);
void clist_node_delete(clist_node_t*, void (*)(clist_node_t*, void*), void*);


#endif /* ! CLIST_H_INCLUDED */
