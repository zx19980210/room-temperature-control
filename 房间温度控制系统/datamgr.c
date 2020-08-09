#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "datamgr.h"
#include "lib/dplist.h"
#include "config.h"
#include "sbuffer.h"
#include "connmgr.h"

void * element_copy_datamgr(void * src_element);
void element_free_datamgr (void ** element);
int element_compare_datamgr(void * x, void * y);

void insert_sensor_map(dplist_t** datamgr,FILE * fp_sensor_map)
{
    *datamgr=dpl_create(element_copy_datamgr,element_free_datamgr,element_compare_datamgr);
    uint16_t* roomID=malloc(sizeof(uint16_t));
    sensor_id_t* sensorID=malloc(sizeof(sensor_id_t));

    ERROR_HANDLER(fp_sensor_map==NULL,"Sensor map file is not existing");

    while(fscanf(fp_sensor_map,"%hu",roomID)!=EOF)
    {
        fscanf(fp_sensor_map,"%hu",sensorID);
        list_element* listElement=malloc(sizeof(list_element));
        listElement->room_ID=*roomID;
        listElement->sensor_ID=*sensorID;
        listElement->last_modified=0;
        listElement->running_avg=0;
        listElement->data_record=calloc(RUN_AVG_LENGTH, sizeof(sensor_value_t));
        dpl_insert_at_index(*datamgr,(void*)listElement,0,0);
    }
    free(roomID);
    free(sensorID);

}

void datamgr_parse_sensor_data(dplist_t* datamgr, sensor_data_t* sensor_data, char* fifo_name)
{

    //start collect data
    ERROR_HANDLER(sensor_data==NULL,"Sensor data is not existing");

    sensor_id_t id=0;
    sensor_value_t data=0;
    sensor_ts_t timestamp=0;
    int size=0;
    size=dpl_size(datamgr);
    //data_amount is used to store the length of sensor data


    id=sensor_data->id;
    data=sensor_data->value;
    timestamp=sensor_data->ts;
    list_element* listElement=NULL;
    sensor_value_t sum=0;
    sensor_value_t* data_array=NULL;
	
    for(int i=0;i<size;i++)
    {
        list_element* ln=(list_element*)dpl_get_element_at_index(datamgr,i);
        if(id==ln->sensor_ID) {
            listElement = ln;
            data_array=listElement->data_record;
            for(int j=0;j<(RUN_AVG_LENGTH-1);j++)
            {
                data_array[j]=data_array[j+1];
                sum+=data_array[j];
            }
            data_array[RUN_AVG_LENGTH-1]=data;
            //update the sensor value
            sum+=data;
        }
        //add data into the list node with the same sensor id
    }
    
    if(listElement!=NULL)
    {
    	listElement->last_modified=timestamp;

	if(data_array[0]!=0)
	    {
		listElement->running_avg=(sensor_value_t)(sum/(sensor_value_t)RUN_AVG_LENGTH);
		print_log(listElement,fifo_name);
	    }
    //check whether the data array is full
    }
    else
    {
	char* send_buffer;
	asprintf(&send_buffer,"Received sensor data with invalid sensor node ID %d\n",id);
	fifo_writer(fifo_name,send_buffer);
	free(send_buffer);

    }

}

void print_log(list_element* listElement,char* fifo_name)
{
    if((listElement->running_avg)>(sensor_value_t)SET_MAX_TEMP)
    {
	char* send_buffer;
	asprintf(&send_buffer,"The sensor node with %d reports it's too hot(running average temperature=%f)\n",listElement->sensor_ID,listElement->running_avg); 
	fifo_writer(fifo_name,send_buffer);
	free(send_buffer);
    }

    else if((listElement->running_avg)<(sensor_value_t)SET_MIN_TEMP)
    {
	char* send_buffer;
	asprintf(&send_buffer,"The sensor node with %d reports it's too cold(running average temperature=%f)\n",listElement->sensor_ID,listElement->running_avg); 
	fifo_writer(fifo_name,send_buffer);
	free(send_buffer);
    }

}

uint16_t datamgr_get_room_id(dplist_t* datamgr,sensor_id_t sensor_id)
{
    int size=dpl_size(datamgr);
    int sensorID=0;
    int roomID=0;
    int result=-1;
    for(int i=0;i<size;i++)
    {
        list_element* listElement=dpl_get_element_at_index(datamgr,i);
        sensorID=listElement->sensor_ID;
        roomID=listElement->room_ID;
        if(sensorID==sensor_id) {
            result = roomID;
        }
    }
    ERROR_HANDLER(result==-1,"INVALID SENSOR ID");
    return result;
}

time_t datamgr_get_last_modified(dplist_t* datamgr, sensor_id_t sensor_id)
{
    int size=dpl_size(datamgr);
    int sensorID=0;
    sensor_ts_t time;
    sensor_ts_t result=-1;
    for(int i=0;i<size;i++)
    {
        list_element* listElement=dpl_get_element_at_index(datamgr,i);
        sensorID=listElement->sensor_ID;
        time=listElement->last_modified;
        if(sensorID==sensor_id) {
            result = time;
        }
    }
    ERROR_HANDLER(result==-1,"INVALID SENSOR ID");
    return result;
}

sensor_value_t datamgr_get_avg(dplist_t* datamgr, sensor_id_t sensor_id)
{
    int size=dpl_size(datamgr);
    int sensorID=0;
    sensor_value_t avg;
    sensor_value_t result=-1;
    for(int i=0;i<size;i++)
    {
        list_element* listElement=dpl_get_element_at_index(datamgr,i);
        sensorID=listElement->sensor_ID;
        avg=listElement->running_avg;
        if(sensorID==sensor_id) {
            result = avg;
        }
    }
    ERROR_HANDLER(result==-1,"INVALID SENSOR ID");
    return result;
}

//result is -1 is the sensor_id is invalid and then show error message

int datamgr_get_total_sensors(dplist_t* datamgr)
{
    return dpl_size(datamgr);
}

void datamgr_free(dplist_t * datamgr)
{
    ERROR_HANDLER(datamgr==NULL,"This datamgr does not exist");
    dpl_free(&datamgr,true);
}

void * element_copy_datamgr(void * src_element)
{
    void * dest=malloc(sizeof(*src_element));
    dest= memcpy(dest, src_element,sizeof(*src_element));
    return dest;
}

void element_free_datamgr (void ** element)
{
    list_element* listElement=(list_element*)*element;
    free(listElement->data_record);
    free(listElement);
    listElement=NULL;
}

int element_compare_datamgr(void * x, void * y)
{
    list_element* listElement1=(list_element*)x;
    list_element* listElement2=(list_element*)y;
    uint16_t roomId_1=listElement1->room_ID;
    uint16_t roomId_2=listElement2->room_ID;
    if(roomId_1==roomId_2)
        return 0;
    else return -1;
    //room ID is unique for each element
}


