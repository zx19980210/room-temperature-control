#ifndef DATAMGR_H_
#define DATAMGR_H_

#include <stdlib.h>
#include <stdio.h>
#include "lib/dplist.h"
#include "config.h"

#define MEMORY_ALLOCATION _ERROR 1 // error due to mem alloc failure
#define INVALID_SENSOR_id 2 //error due to invalid sensor id
#define NON_EXISTING_FILE 3 //error due to non-existing files

#ifndef RUN_AVG_LENGTH
  #define RUN_AVG_LENGTH 5
#endif




#ifndef SET_MAX_TEMP
#error SET_MAX_TEMP not set
#endif

#ifndef SET_MIN_TEMP
#error SET_MIN_TEMP not set
#endif


/*
 * Use ERROR_HANDLER() for handling memory allocation problems, invalid sensor IDs, non-existing files, etc.
 */
#define ERROR_HANDLER(condition,...) 	do { \
					  if (condition) { \
					    printf("\nError: in %s - function %s at line %d: %s\n", __FILE__, __func__, __LINE__, __VA_ARGS__); \
					    exit(EXIT_FAILURE); \
					  }	\
					} while(0)

/*
 * This method should be called to clean up the datamgr, and to free all used memory. 
 * After this, any call to datamgr_get_room_id, datamgr_get_avg, datamgr_get_last_modified or datamgr_get_total_sensors will not return a valid result
 */
void datamgr_free(dplist_t* datamgr);
    
/*   
 * Gets the room ID for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid 
 */
uint16_t datamgr_get_room_id(dplist_t* datamgr,sensor_id_t sensor_id);


/*
 * Gets the running AVG of a certain senor ID (if less then RUN_AVG_LENGTH measurements are recorded the avg is 0)
 * Use ERROR_HANDLER() if sensor_id is invalid 
 */
sensor_value_t datamgr_get_avg(dplist_t* datamgr,sensor_id_t sensor_id);


/*
 * Returns the time of the last reading for a certain sensor ID
 * Use ERROR_HANDLER() if sensor_id is invalid 
 */
time_t datamgr_get_last_modified(dplist_t* datamgr,sensor_id_t sensor_id);


/*
 *  Return the total amount of unique sensor ID's recorded by the datamgr
 */
int datamgr_get_total_sensors(dplist_t* datamgr);

/* Insert sensor data into datamgr
 * Send log message when the temperature is too hot or cold
 */
void datamgr_parse_sensor_data(dplist_t* datamgr, sensor_data_t* sensor_data,char* fifo_name);

/* Insert room ID and sensor ID that are recorded in fp_sensor_map*/
void insert_sensor_map(dplist_t** datamgr,FILE * fp_sensor_map);

/* Write message into fifo based the average temperature*/
void print_log(list_element* listElement,char* fifo_name);



#endif  //DATAMGR_H_

