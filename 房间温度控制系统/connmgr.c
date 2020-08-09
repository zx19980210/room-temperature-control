#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <inttypes.h>
#include <assert.h>
#include <time.h>
#include <sys/socket.h>
#include "config.h"
#include "lib/tcpsock.h"
#include "lib/dplist.h"
#include "connmgr.h"
#include <poll.h>
#include <string.h>
#include "sbuffer.h"
#include <pthread.h>
#include "errmacros.h"

#ifdef DEBUG
	#define CONN_DEBUG_PRINTF(condition,...)									\
		do {												\
		   if((condition)) 										\
		   {												\
			fprintf(stderr,"\nIn %s - function %s at line %d: ", __FILE__, __func__, __LINE__);	\
			fprintf(stderr,__VA_ARGS__);								\
		   }												\
		} while(0)						
#else
	#define TCP_DEBUG_PRINTF(...) (void)0
#endif
		  

#define CONN_ERR_HANDLER(condition,...)	\
	do {						\
		if ((condition))			\
		{					\
		  CONN_DEBUG_PRINTF(1,"error condition \"" #condition "\" is true\n");	\
		  __VA_ARGS__;				\
		}					\
	} while(0)

#ifndef TIMEOUT
#error TIMEOUT is not set
#endif

void * element_copy(void * src_element);
void element_free_sensor (void ** element);
void element_free_connmgr (void ** element);
int element_compare_sensor(void * x, void * y);
int element_compare_connmgr(void * x, void * y);

dplist_t* connmgr=NULL;
//data structure for all sockets
int flag=0;


void fifo_writer(char* file_path,char* send_buffer)
{
	FILE * writer_fifo=fopen(file_path,"w");
	FILE_OPEN_ERROR(writer_fifo);

	char* sending_msg;
	time_t time_now=time(NULL);
	struct tm* tm;
	tm=localtime(&time_now);
	asprintf(&sending_msg,"Timestamp: %d.%d.%d %d:%d:%d.%s",1900+tm->tm_year,
		1+tm->tm_mon,
		tm->tm_mday,
		tm->tm_hour,
		tm->tm_min,
		tm->tm_sec,
		send_buffer);
	
	if ( fputs( sending_msg, writer_fifo ) == EOF )
    	{
     	 	fprintf( stderr, "Error writing data to fifo\n");
      	 	exit( EXIT_FAILURE );
    	} 
	FFLUSH_ERROR(fflush(writer_fifo));
	int result=fclose(writer_fifo);
	FILE_CLOSE_ERROR(result);
	free(sending_msg);
	
}

void connmgr_free()
{
	dpl_free(&connmgr,true);
}

int connmgr_check_sensor_node(dplist_t* sensor_list,int sensor_id)
{
	int size=dpl_size(sensor_list);
	int result=0;
	if(size==0)
		return result;
	for(int i=0;i<size;i++)
	{
		int s_id=((sensor_node_element*)dpl_get_element_at_index(sensor_list,i))->sensor_id;
		if(s_id==sensor_id)
		{
			result=1;
			break;
		}
	}
	return result;
		
}
//return value equals 1 means this sensor is existed in the list

void connmgr_disconnect_sensor_node(dplist_t* sensor_list,char* fifo_name,pthread_mutex_t* mutex_lock)
{
	time_t time_now=time(NULL);
	time_t last_active_ts;
	int size=dpl_size(sensor_list);
	int sensor_id;
	if(size>0)
	{
		for(int i=0;i<size;i++)
		{
			sensor_node_element* sensor=(sensor_node_element*)dpl_get_element_at_index(sensor_list,i);
			last_active_ts=sensor->last_active_ts;
			sensor_id=sensor->sensor_id;
			if((time_now-last_active_ts)>TIMEOUT)
			{
				dpl_remove_at_index(sensor_list,i,true);

				char* send_buffer;
				asprintf(&send_buffer,"The sensor node with %d has closed the connection\n",sensor_id);
				pthread_mutex_lock(mutex_lock);
				fifo_writer(fifo_name,send_buffer);
				pthread_mutex_unlock(mutex_lock);
				free(send_buffer);
			}
		}
	}
}

void connmgr_remove_socket_by_port(dplist_t* connmgr, int port_number)
{
	int size=dpl_size(connmgr);
	socket_t* dummy=NULL;
	for(int i=0;i<size;i++)
	{
		dummy=(socket_t*)dpl_get_element_at_index(connmgr,i);
		if(port_number==dummy->port_number)
			break;
	}
	int index=dpl_get_index_of_element(connmgr,(void*)dummy);
	dpl_remove_at_index(connmgr,index,true);
	
}

sensor_node_element* connmgr_find_sensor_node_by_id(dplist_t* sensor_list, int sensor_id)
{
	int size=dpl_size(sensor_list);
	int s_id;
	sensor_node_element* result=NULL;
	sensor_node_element* dummy=NULL;
	for(int i=0;i<size;i++)
	{
		dummy=(sensor_node_element*)dpl_get_element_at_index(sensor_list,i);
		s_id=dummy->sensor_id;
		if(s_id==sensor_id)
		{
			result=dummy;
			break;
		}
	}
	return result;
}
//find corresponding sensor node by sensor id	

void connmgr_listen(int port_number, sbuffer_t* buffer, int* counter,pthread_mutex_t* mutex_lock,char* fifo_name)
{
	if(flag==0)
	{
	    connmgr=dpl_create(element_copy,element_free_connmgr,element_compare_connmgr);
	    flag=1;
	}
	//create new connmgr if it is not existed

	dplist_t* sensor_list=dpl_create(element_copy,element_free_sensor,element_compare_sensor);
	//sensor_list is used to store the connected sensor of this port

	tcpsock_t* server=NULL;
	tcpsock_t* client=NULL;
	int connected_flag=0;
	//connected_flag is used to indicate whethre client is allocated
	int connmgr_size=0;
	int activities=0;
	int fds_size=0;

  	if (tcp_passive_open(&server,port_number)!=TCP_NO_ERROR) 
		exit(EXIT_FAILURE);

	//initialize struct socket_t
	socket_t * socket=(socket_t*)malloc(sizeof(socket_t));
	socket->sd=server->sd;
	socket->port_number=port_number;
	
	connmgr_size=dpl_size(connmgr);
	int connmgr_result=0;
	for(int i=0;i<connmgr_size;i++)
	{
		socket_t* dummy=(socket_t*)dpl_get_element_at_index(connmgr,i);
		int port_n=dummy->port_number;
		if(port_n==port_number)
		{
			connmgr_result=1;
			break;
		}
	}
	if(connmgr_result==0)
	dpl_insert_at_index(connmgr,(void*)socket,0,false);
	else
	free(socket);

	struct pollfd* fds=calloc(1,sizeof(struct pollfd));//one for each sensor and one to listen for new connection
	
	fds[0].fd=server->sd;
	fds[0].events=POLLIN;
	fds_size=1;
	while(1)
	{
		activities=poll(fds,fds_size,TIMEOUT*1000);
		//detect activities of this port

		if(activities<0)//error occurs
		{
			perror("error!\n");
			exit(EXIT_FAILURE);
		}

		if(activities==0)
		//no activities on the socket, socket is closed an remove from the connmgr
		{
			if(dpl_size(sensor_list)!=0)
			{
				for(int i=0;i<dpl_size(sensor_list);i++)
				{
					int id=((sensor_data_t*)dpl_get_element_at_index(sensor_list,i))->id;
					char* send_buffer;
					asprintf(&send_buffer,"The sensor node with %d has closed the connection\n",id);
					pthread_mutex_lock(mutex_lock);
					fifo_writer(fifo_name,send_buffer);
					pthread_mutex_unlock(mutex_lock);
					free(send_buffer);
				}
				dpl_free(&sensor_list,true);
			}
			else
			{
				free(sensor_list);
				sensor_list=NULL;
			}

			connmgr_remove_socket_by_port(connmgr,port_number);
			free(fds);
			tcp_close(&server);
			if(connected_flag==1)
			tcp_close(&client);

			char* send_buffer;
			asprintf(&send_buffer,"Port %d has closed\n",port_number);
			pthread_mutex_lock(mutex_lock);
			fifo_writer(fifo_name,send_buffer);
			pthread_mutex_unlock(mutex_lock);
			free(send_buffer);
			
			break;
		}

		
		if(activities>0&&(fds[0].revents&POLLIN)==POLLIN)//detect new connection to this port
		{
			
			if(client!=NULL)
			{
				free(client->ip_addr);
				free(client);
				client=NULL;
			}

			if (tcp_wait_for_connection(server,&client)!=TCP_NO_ERROR)
				exit(EXIT_FAILURE);

			//add new sd into sd array
			struct pollfd* fds_new=calloc(fds_size+1,sizeof(struct pollfd));
			for(int i=0;i<fds_size;i++)
			{
				fds_new[i].fd=fds[i].fd;
				fds_new[i].events=POLLIN;
			}
			fds_new[fds_size].fd=client->sd;
			fds_new[fds_size].events=POLLIN;
			fds_size++;
			free(fds);
			fds=fds_new;
			connected_flag=1;
				
			if(--activities<=0)
				continue;
		}

		//start reading data
		for(int index=1;index<fds_size;index++)
		{
			
			int result,bytes;
			sensor_data_t data;
			
			if(activities>0&&fds[index].revents==POLLIN)
			{
		
				//read id
				bytes=sizeof(data.id);
				result=tcp_receive_by_sd(fds[index].fd,(void*)&data.id,&bytes);
				//read temperature
				bytes=sizeof(data.value);
				result=tcp_receive_by_sd(fds[index].fd,(void*)&data.value,&bytes);
				//read timestamp
				bytes=sizeof(data.ts);
				result=tcp_receive_by_sd(fds[index].fd,(void*)&data.ts,&bytes);			

				//add new sensor node into sensor list
				sensor_node_element * s=(sensor_node_element*)malloc(sizeof(sensor_node_element));
				s->sensor_id=data.id;
				s->last_active_ts=-1;
				//default value is -1;
				
				pthread_mutex_lock(mutex_lock);
				//add sensor_node into sensor list when new connection is coming
				if(connmgr_check_sensor_node(sensor_list,data.id)==0)
				{
					s->last_active_ts=time(NULL);
					dpl_insert_at_index(sensor_list,(void*)s,0,false);

					char* send_buffer;
					asprintf(&send_buffer,"A sensor node with %d has opened a new connection\n",data.id);
					fifo_writer(fifo_name,send_buffer);
					free(send_buffer);
				}
				else
				{
					free(s);
					sensor_node_element * sensor=connmgr_find_sensor_node_by_id(sensor_list,data.id);
					sensor->last_active_ts=time(NULL);
				}

			      if ((result==TCP_NO_ERROR) && bytes) 
			      {
				
				connmgr_write_data(data.id,data.value,data.ts,buffer,counter);
				
			      }
			      pthread_mutex_unlock(mutex_lock);
			      			
			      if(result==TCP_CONNECTION_CLOSED)
				{
					//delete this invalid sd from fds
					struct pollfd* new_fds=(struct pollfd*)calloc(fds_size-1,sizeof(struct pollfd));
					for(int i=0,j=0;i<fds_size;i++)
					{
						if(fds[i].fd!=fds[index].fd)
						{
							new_fds[j].fd=fds[i].fd;
							new_fds[j].events=POLLIN;
							j++;
						}
					}
					free(fds);
					fds=new_fds;
					fds_size--;
					//break;
				}
				

				if(--activities==0)
					break;	
			}
		}

		connmgr_disconnect_sensor_node(sensor_list,fifo_name,mutex_lock);
		
				
			
	}
	connmgr_free();
}	

void connmgr_write_data(sensor_id_t id,sensor_value_t value,sensor_ts_t ts,sbuffer_t* buffer,int * counter)
{
	FILE* file=NULL;
	file=fopen("sensor_data_recv","ab+");
	if(file==NULL)
		exit(EXIT_FAILURE);
	sensor_id_t id_rcv=id;
	sensor_value_t value_rcv=value;
	sensor_ts_t ts_rcv=ts;
	
	sensor_data_t *data=malloc(sizeof(sensor_data_t));
	data->id=id_rcv;
	data->value=value_rcv;
	data->ts=ts_rcv;
	int result=sbuffer_insert(buffer,data);
	if(result==SBUFFER_SUCCESS)
		(*counter)++;
	
	fwrite(&id_rcv,sizeof(sensor_id_t),1,file);
	fwrite(&value_rcv,sizeof(sensor_value_t),1,file);
	fwrite(&ts_rcv,sizeof(sensor_ts_t),1,file);
	fclose(file);
	free(data);
}
		

/*callback function for sensor_list and connmgr*/
void * element_copy(void * src_element)
{
    void * dest=malloc(sizeof(*src_element));
    dest= memcpy(dest, src_element,sizeof(*src_element));
    return dest;
}


void element_free_connmgr (void ** element)
{
    socket_t* socket=(socket_t*)*element;
    free(socket);
    socket=NULL;
  
}

void element_free_sensor (void** element)
{
  sensor_node_element* sensor_node=(sensor_node_element*)*element;
  free(sensor_node);
  sensor_node=NULL;
}

int element_compare_connmgr(void * x, void * y)
{
  socket_t* socket_1=(socket_t*)x;
  socket_t* socket_2=(socket_t*)y;
  int port_1=socket_1->port_number;
  int port_2=socket_2->port_number;
  if(port_1==port_2)
	return 0;
  else return 1;
}

int element_compare_sensor(void * x, void * y)
{
  sensor_node_element* sensor_1=(sensor_node_element*)x;
  sensor_node_element* sensor_2=(sensor_node_element*)y;
  int sensor_id_1=sensor_1->sensor_id;
  int sensor_id_2=sensor_2->sensor_id;
  if(sensor_id_1==sensor_id_2)
	return 0;
  else return 1;
}
