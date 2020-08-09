#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "dplist.h"

/*
 * definition of error codes
 * */
#define DPLIST_NO_ERROR 0
#define DPLIST_MEMORY_ERROR 1 // error due to mem alloc failure
#define DPLIST_INVALID_ERROR 2 //error due to a list operation applied on a NULL list 

#ifdef DEBUG
	#define DEBUG_PRINTF(...) 									         \
		do {											         \
			fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	 \
			fprintf(stderr,__VA_ARGS__);								 \
			fflush(stderr);                                                                          \
                } while(0)
#else
	#define DEBUG_PRINTF(...) (void)0
#endif


#define DPLIST_ERR_HANDLER(condition,err_code)\
	do {						            \
            if ((condition)) DEBUG_PRINTF(#condition " failed\n");    \
            assert(!(condition));                                    \
        } while(0)

        
/*
 * The real definition of struct list / struct node
 */

struct dplist_node {
  dplist_node_t * prev, * next;
  void * element;
};

struct dplist {
  dplist_node_t * head;
  void * (*element_copy)(void * src_element);			  
  void (*element_free)(void ** element);
  int (*element_compare)(void * x, void * y);
};


dplist_t * dpl_create (// callback functions
			  void * (*element_copy)(void * src_element),
			  void (*element_free)(void ** element),
			  int (*element_compare)(void * x, void * y)
			  )
{
  dplist_t * list;
  list = malloc(sizeof(struct dplist));
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_MEMORY_ERROR);
  list->head = NULL;  
  list->element_copy = element_copy;
  list->element_free = element_free;
  list->element_compare = element_compare; 
  return list;
}

void dpl_free(dplist_t ** list, bool free_element)
{
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  int size=dpl_size(*list);
  dplist_node_t * node_tobe_deleted;
  
  for(int count=size-1;count>=0;count--)
  {
    node_tobe_deleted=dpl_get_reference_at_index(*list,count);
    dpl_remove_at_index(*list, count,free_element);
    //free(node_tobe_deleted);
  }
  free(*list);
  *list=NULL;
}

dplist_t * dpl_insert_at_index(dplist_t * list, void * element, int index, bool insert_copy)
{
  if(element==NULL)
  return list;

  void * new_element;
  dplist_node_t * ref_at_index, * list_node;
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  list_node = malloc(sizeof(dplist_node_t));
  DPLIST_ERR_HANDLER(list_node==NULL,DPLIST_MEMORY_ERROR);
  if(insert_copy==true)
  {
    void * (*copy)(void * src_element)=list->element_copy;
    new_element=copy(element);
  }
  else
    new_element=element;
    
  list_node->element=new_element;

   if (list->head == NULL)  
  //check whether it is the first element
  { // empty list ==> avoid errors
    list_node->prev = NULL;
    list_node->next = NULL;
    list->head = list_node;
    // pointer drawing breakpoint
  } else if (index <= 0)  
  //do operation at the start of the list 
  { // covers case 2 
    list_node->prev = NULL;
    list_node->next = list->head;
    list->head->prev = list_node;
    list->head = list_node;
    // pointer drawing breakpoint
    //four pointers should be altered
  } else 
  {
    ref_at_index = dpl_get_reference_at_index(list, index);  
    assert( ref_at_index != NULL);
    //check if it's out of boundary
    // pointer drawing breakpoint
    if (index < dpl_size(list))
    { // do operation at the middle of the list 
      list_node->prev = ref_at_index->prev;
      list_node->next = ref_at_index;
      ref_at_index->prev->next = list_node;
      ref_at_index->prev = list_node;
      // pointer drawing breakpoint
    } else
    { //  do operation at the end of the list 
      assert(ref_at_index->next == NULL);
      list_node->next = NULL;
      list_node->prev = ref_at_index;
      ref_at_index->next = list_node;    
      // pointer drawing breakpoint
      //ref_at_index is the last one in the list
    }
  }
  return list;
}

dplist_t * dpl_remove_at_index( dplist_t * list, int index, bool free_element)
{
    DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);

    if(list->head==NULL)
        return list;

    void (*e_free)(void ** element)=list->element_free;
    dplist_node_t * ref_at_index=dpl_get_reference_at_index(list, index);
    void** element_ptr=&(ref_at_index->element);
    if(free_element==true)
        e_free(element_ptr);

    if(index<=0)
    {
        if(ref_at_index->next!=NULL)
        {
            list->head=ref_at_index->next;
            ref_at_index->next->prev=NULL;
        }
        else
        {
            list->head=NULL;
        }
    }
    else if (index<dpl_size(list)-1)
    {
        ref_at_index->prev->next=ref_at_index->next;
        ref_at_index->next->prev=ref_at_index->prev;
    }
    else{
        if(ref_at_index->prev!=NULL)
            ref_at_index->prev->next=NULL;
        else
            list->head=NULL;
    }
    free(ref_at_index);
    return list;
}

int dpl_size( dplist_t * list )
{

DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  if(list->head==NULL)
  return 0;

  int count=1;
  dplist_node_t* dummy=list->head;

  
  while(dummy->next!=NULL)
    {
      dummy=dummy->next;
      count++;
    }
  return count;
}

void * dpl_get_element_at_index( dplist_t * list, int index )
{
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  if(list->head==NULL)
  return (void*)0;

  dplist_node_t* node =dpl_get_reference_at_index(list, index);
  void* element=node->element;
  return element;
}

int dpl_get_index_of_element( dplist_t * list, void * element )
{
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  if(element==NULL)
  return -1;
  int result=-1;
  int (*compare)(void * x, void * y)=list->element_compare;
  for(int count=0;count<dpl_size(list);count++)
  {
    dplist_node_t* node=dpl_get_reference_at_index(list, count);
    if(compare(element,node->element)==0)
    {
      result=count;
      break;
    }
  }
  return result;
}

dplist_node_t * dpl_get_reference_at_index( dplist_t * list, int index )
{
  int count;
  dplist_node_t * dummy;
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  if (list->head == NULL ) return NULL;
  for ( dummy = list->head, count = 0; dummy->next != NULL ; dummy = dummy->next, count++) 
  { 
    if (count >= index) return dummy;
  }  
  return dummy;  
}

dplist_node_t * dpl_get_first_reference( dplist_t * list )
{
  dplist_node_t * dummy;
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  if(list->head==NULL)
  return NULL;
  dummy=list->head;
  return dummy;
}

dplist_node_t * dpl_get_last_reference( dplist_t * list )
{
  dplist_node_t * dummy=NULL;
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  if(list->head==NULL)
  return NULL;
  int size=dpl_size(list);
  dummy=list->head;
  for(int count=1;count<size;count++)
  {
    dummy=dummy->next;
  }
  return dummy;
}

bool check_existing(dplist_t * list, dplist_node_t* reference)
{
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  dplist_node_t* dummy=NULL;
  if(reference==NULL||list->head==NULL)
  return false;

  bool result=false;
  int size=dpl_size(list);
  dummy=list->head;
  for(int count=1;count<size;count++)
  {
    if(dummy==reference)
    {
    result=true;
    break;
    }
    dummy=dummy->next;
  }
  return result;
}

dplist_node_t * dpl_get_next_reference( dplist_t * list, dplist_node_t * reference )
{
  bool result=check_existing(list, reference);
  if(result==false)
  return NULL;

  else
    return reference->next;
  
}

dplist_node_t * dpl_get_previous_reference( dplist_t * list, dplist_node_t * reference )
{
  bool result=check_existing(list, reference);
  if(result==false)
  return NULL;
  else{
    return reference->prev;
  }
}
 
void * dpl_get_element_at_reference( dplist_t * list, dplist_node_t * reference )
{
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
    bool result=false;
    int size=dpl_size(list);

    if(list->head==NULL)
        return NULL;

    if(reference==NULL)
    {
        dplist_node_t* dummy=dpl_get_last_reference(list);
        return dummy->element;
    }
    else
    {
        dplist_node_t* ptr=list->head;
        for(int count=1;count<size;count++)
        {
            if(ptr==reference)
            {
                result=true;
                break;
            }
            ptr=ptr->next;
        }

        if(result==false)
        return NULL;
        else return reference->element;
    }
}

dplist_node_t * dpl_get_reference_of_element( dplist_t * list, void * element )
{
  DPLIST_ERR_HANDLER(list==NULL,DPLIST_INVALID_ERROR);
  if(list->head==NULL)
  return NULL;

  else{
    int (*compare)(void * x, void * y)=list->element_compare;
    int size=dpl_size(list);
    dplist_node_t* result=NULL;
    dplist_node_t* dummy=list->head;
    for(int count=1;count<size;count++)
    {
      if(compare(element,dummy->element)==0)
      {
        result=dummy;
        break;
      }
      dummy=dummy->next;
    }
    return result;
  }
}




