/*
** list.h --- list control
*/
#ifndef LIST_H
#define LIST_H

struct list_head {
  struct list_head *next;
  struct list_head *prev;
};

#define list_init(head)\
	{(head)->prev = (head)->next = (head);}

#define list_alloc(head, size, idx)\
	{ unsigned int head##_idx; idx=0;\
          for(head##_idx=1;head##_idx<size;head##_idx++)\
            {if(head[head##_idx].status==0)\
               {idx=head##_idx; head[head##_idx].status=1;break;} }}\

#define list_empty(head)\
	((head)->next == (head))

#define list_add(head, item)\
	{\
	  (item)->next = (head)->next;\
	  (item)->prev = (head);\
	  (head)->next->prev = (item);\
	  (head)->next = (item);\
	}

#define list_add_tail(head, item)\
	{\
	  (item)->next = (head);\
	  (item)->prev = (head)->prev;\
	  (head)->prev->next = (item);\
	  (head)->prev = (item);\
	}
#define list_insert_prev list_add_tail
#define list_add_next    list_add

#define list_del(cur)\
	{\
	  (cur)->prev->next = (cur)->next;\
	  (cur)->next->prev = (cur)->prev;\
	  (cur)->prev = (cur)->next = (cur);\
	}

#define list_for_each( head, item )\
  for( (item) = (head)->next; (item) != (head); (item) = (item)->next )

#endif
