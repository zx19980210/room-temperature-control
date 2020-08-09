#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <stdint.h>
#include <time.h>
#include "lib/dplist.h"
#include "lib/tcpsock.h"

typedef uint16_t sensor_id_t;
typedef double sensor_value_t;     
typedef time_t sensor_ts_t;         // UTC timestamp as returned by time() - notice that the size of time_t is different on 32/64 bit machine

//list element saved in sensor_list
typedef struct{
	sensor_id_t id;
	sensor_value_t value;
	sensor_ts_t ts;
} sensor_data_t;

typedef struct{
	int sd;
	int port_number;
} socket_t;

typedef struct{
	sensor_id_t sensor_id;
	sensor_ts_t last_active_ts;
} sensor_node_element;

typedef struct{
    sensor_id_t sensor_ID;
    uint16_t  room_ID;
    sensor_value_t running_avg;
    sensor_ts_t last_modified;
    sensor_value_t* data_record;
}list_element;

typedef struct sbuffer_node {
  struct sbuffer_node * next;
  struct sbuffer_node * prev;
  sensor_data_t data;
  int read_by_data_mgr;
  int read_by_storage_mgr;
} sbuffer_node_t;

struct sbuffer {
  sbuffer_node_t * head;
  sbuffer_node_t * tail;
};

typedef struct sbuffer sbuffer_t;
	
			

#endif /* _CONFIG_H_ */

