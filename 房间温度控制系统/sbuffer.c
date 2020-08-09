#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include "sbuffer.h"


int sbuffer_init(sbuffer_t ** buffer)
{
  *buffer = malloc(sizeof(sbuffer_t));
  if (*buffer == NULL) return SBUFFER_FAILURE;
  (*buffer)->head = NULL;
  (*buffer)->tail = NULL;
  return SBUFFER_SUCCESS; 
}


int sbuffer_free(sbuffer_t ** buffer)
{
  sbuffer_node_t * dummy;
  if ((buffer==NULL) || (*buffer==NULL)) 
  {
    return SBUFFER_FAILURE;
  } 
  while ( (*buffer)->head )
  {
    dummy = (*buffer)->head;
    (*buffer)->head = (*buffer)->head->next;
    free(dummy);
  }
  free(*buffer);
  *buffer = NULL;	
 
  return SBUFFER_SUCCESS;		
}


int sbuffer_remove(sbuffer_t * buffer,sensor_data_t * data)
{
  sbuffer_node_t * dummy;
  if (buffer == NULL) return SBUFFER_FAILURE;
  if (buffer->head == NULL) return SBUFFER_NO_DATA;
  *data = buffer->head->data;
  dummy = buffer->head;

  if (buffer->head == buffer->tail) // buffer has only one node
  {
    buffer->head = buffer->tail = NULL; 
  }
  else  // buffer has many nodes empty
  {
    buffer->head = buffer->head->next;
  }
  free(dummy);
  return SBUFFER_SUCCESS;
}


int sbuffer_insert(sbuffer_t * buffer, sensor_data_t * data)
{
  sbuffer_node_t * dummy;
  if (buffer == NULL) return SBUFFER_FAILURE;
  dummy = malloc(sizeof(sbuffer_node_t));
  if (dummy == NULL) return SBUFFER_FAILURE;
  dummy->data = *data;
  dummy->next = NULL;
  dummy->prev = NULL;
  dummy->read_by_data_mgr=SBUFFER_NOT_READ;
  dummy->read_by_storage_mgr=SBUFFER_NOT_READ;
  if (buffer->tail == NULL) // buffer empty (buffer->head should also be NULL
  {
    buffer->head = buffer->tail = dummy;
  } 
  else // buffer not empty
  {
    buffer->tail->next = dummy;
    dummy->prev=buffer->tail;
    buffer->tail = buffer->tail->next; 
  }
  return SBUFFER_SUCCESS;
}

int sbuffer_size(sbuffer_t *buffer)
{
	if (buffer == NULL) return SBUFFER_FAILURE;
	if (buffer->tail==NULL)
		return 0;

	int size=1;
	sbuffer_node_t * dummy;
	dummy=buffer->head;
	while(dummy!=buffer->tail)
	{
		dummy=dummy->next;
		size++;	
	}
	return size;
}


sensor_data_t* sbuffer_read_by_index(sbuffer_t * buffer,int reader, int index)
{
	if (buffer == NULL) return NULL;
	int size=sbuffer_size(buffer);
	if(buffer->tail==NULL)//empty buffer
	{
		return NULL;
	}
	
	if(index<=0&&reader==DATA_MGR&&buffer->head->read_by_data_mgr==SBUFFER_NOT_READ)
	{
		buffer->head->read_by_data_mgr=SBUFFER_READ;
		return &(buffer->head->data);
	}

	if(index<=0&&reader==STORAGE_MGR&&buffer->head->read_by_storage_mgr==SBUFFER_NOT_READ)
	{
		buffer->head->read_by_storage_mgr=SBUFFER_READ;
		return &(buffer->head->data);
	}

	if(index>=size&&reader==DATA_MGR&&buffer->tail->read_by_data_mgr==SBUFFER_NOT_READ)
	{
		buffer->tail->read_by_data_mgr=SBUFFER_READ;
		return &(buffer->tail->data);
	}

	if(index>=size&&reader==STORAGE_MGR&&buffer->tail->read_by_storage_mgr==SBUFFER_NOT_READ)
	{
		buffer->tail->read_by_storage_mgr=SBUFFER_READ;
		return &(buffer->tail->data);
	}
	
	sbuffer_node_t * dummy=NULL;
	dummy=sbuffer_get_reference_at_index(buffer,index);

	if(dummy==NULL) return NULL;
	
	//check whether it is readable
	if(reader==DATA_MGR&&dummy->read_by_data_mgr==SBUFFER_NOT_READ)
	{
		dummy->read_by_data_mgr=SBUFFER_READ;
		return &(dummy->data);
	}

	if(reader==STORAGE_MGR&&dummy->read_by_storage_mgr==SBUFFER_NOT_READ)
	{
		dummy->read_by_storage_mgr=SBUFFER_READ;
		return &(dummy->data);
	}

	return NULL;
}

sbuffer_node_t* sbuffer_get_reference_at_index(sbuffer_t * buffer, int index)
{
	if (buffer == NULL) return NULL;
	
	if(buffer->tail==NULL)//empty buffer
	{
		return NULL;
	}

	int size=sbuffer_size(buffer);
	if(index<=0)
		return buffer->head;
	if(index>=size)
		return buffer->tail;

	sbuffer_node_t * dummy;
	dummy=buffer->head;
	for(int i=0;i<size;i++)
	{
		if(i>=index)
			break;
		else
			dummy=dummy->next;
	}
	return dummy;
}


int sbuffer_remove_processed_node(sbuffer_t * buffer)
{
	if (buffer == NULL) return SBUFFER_FAILURE;
	int size=sbuffer_size(buffer);
	
	if(buffer->tail==NULL)//empty buffer
	{
		return SBUFFER_NO_DATA;
	}

	sbuffer_node_t* dummy=buffer->head;
	sbuffer_node_t* node_to_be_freed=NULL;
	for(int i=0;i<size;i++)
	{
		//if this node is processed by both data mgr and storage mgr
		if(dummy->read_by_data_mgr==SBUFFER_READ&&dummy->read_by_storage_mgr==SBUFFER_READ)
		{
			//only one remaining element
			if(sbuffer_size(buffer)==1)
			{
				buffer->head=NULL;
				buffer->tail=NULL;
				free(dummy);
				dummy=NULL;
				return SBUFFER_SUCCESS;
			}
			
			//dummy is at the start of current buffer
			else if(dummy->prev==NULL)
			{
				dummy->next->prev=NULL;
				buffer->head=dummy->next;
			}
				
			//dummy is at the end of current buffer
			else if (dummy->next==NULL)
			{
				dummy->prev->next=NULL;
				buffer->tail=dummy->prev;
			}

			else
			{
				dummy->prev->next=dummy->next;
				dummy->next->prev=dummy->prev;
			}
			node_to_be_freed=dummy;
			dummy=dummy->next;
			free(node_to_be_freed);
			node_to_be_freed=NULL;
			continue;

		}
		dummy=dummy->next;
	}
	
	return SBUFFER_SUCCESS;
}



