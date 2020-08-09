#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include "lib/dplist.h"
#include "config.h"
#include "sbuffer.h"
#include <pthread.h>

#define	CONN_NO_ERROR		0
#define CONN_FILE_ERROR		1

#define ERROR_HANDLER(condition,...) 	do { \
					  if (condition) { \
					    printf("\nError: in %s - function %s at line %d: %s\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)
/*write message into fifo in order to communicate between two process*/
void fifo_writer(char* file_path,char* send_buffer);

/*listen for connection based on given port_number
 *write received data into shared buffer
 *mutex lock is received from main function
 */
void connmgr_listen(int port_number,sbuffer_t* buffer, int* counter,pthread_mutex_t* mutex_lock,char* fifo_name);


/*free all allocated memory*/
void connmgr_free();

/*check whether the sensor with sensor_id is already existed in sensor_list
 *return value is 1 if it's existed
 */
int connmgr_check_sensor_node(dplist_t* sensor_list,int sensor_id);
 
/*find sensor node based on given sensor_id*/
sensor_node_element* connmgr_find_sensor_node_by_id(dplist_t* sensor_list, int sensor_id);

/*disconnect these invalid sensor node and remove it from sensor_list if there is no data received from this sensor after 2*TIMEOUT
 */
void connmgr_disconnect_sensor_node(dplist_t* sensor_list,char* fifo_name,pthread_mutex_t* mutex_lock);

/*remove socket from connmgr based on given port_number*/
void connmgr_remove_socket_by_port(dplist_t* connmgr, int port_number);

/*write data into shared buffer
 *counter represent the amount of data sent into shared buffer
 */
void connmgr_write_data(sensor_id_t id,sensor_value_t value,sensor_ts_t ts, sbuffer_t* buffer,int *counter);



