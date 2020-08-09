#ifndef _SBUFFER_H_
#define _SBUFFER_H_

#include "config.h"

#define SBUFFER_FAILURE -1
#define SBUFFER_SUCCESS 0
#define SBUFFER_NO_DATA 1
#define SBUFFER_READ 1
#define SBUFFER_NOT_READ 0
#define DATA_MGR 1
#define STORAGE_MGR 0
#define SBUFFER_NOT_READABLE 2 

/*
 * Allocates and initializes a new shared buffer
 * Returns SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
 */
int sbuffer_init(sbuffer_t ** buffer);


/*
 * All allocated resources are freed and cleaned up
 * Returns SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
 */
int sbuffer_free(sbuffer_t ** buffer);


/*
 * Removes the first sensor data in 'buffer' (at the 'head') and returns this sensor data as '*data'  
 * 'data' must point to allocated memory because this functions doesn't allocated memory
 * If 'buffer' is empty, the function doesn't block until new sensor data becomes available but returns SBUFFER_NO_DATA
 * Returns SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
 */
int sbuffer_remove(sbuffer_t * buffer,sensor_data_t * data);


/* Inserts the sensor data in 'data' at the end of 'buffer' (at the 'tail')
 * Returns SBUFFER_SUCCESS on success and SBUFFER_FAILURE if an error occured
 */
int sbuffer_insert(sbuffer_t * buffer, sensor_data_t * data);


/* Get size of current buffer*/
int sbuffer_size(sbuffer_t *buffer);

/*Remove sbuffer node from buffer based on its position in the list*/
int sbuffer_remove_at_index(sbuffer_t * buffer, int index);

/*Remove sbuffer nodes that are already read by both storage manager and data manager*/
int sbuffer_remove_processed_node(sbuffer_t * buffer);

/*Get specific sbuffer node in the buffer based the position*/
sbuffer_node_t* sbuffer_get_reference_at_index(sbuffer_t * buffer, int index);

//int sbuffer_read_by_index(sbuffer_t * buffer, sensor_data_t* data, int reader, int index);
sensor_data_t* sbuffer_read_by_index(sbuffer_t * buffer,int reader, int index);

#endif  //_SBUFFER_H_

