#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <assert.h>
#include "sensor_db.h"
#include "sbuffer.h"
#include "config.h"
#include "connmgr.h"
#include "datamgr.h"
#include <unistd.h>
#include <pthread.h>
#include <sqlite3.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "errmacros.h"

#define TIME_WAIT 1
#define FIFO_NAME "logFifo"
#define LOG_FILE_NAME "gateway.log"
#define MAX 150

pthread_mutex_t mutex_lock;
sbuffer_t* buffer=NULL;
dplist_t* datamgr=NULL;
int port;
int * counter;
int writer_finish_flag=0;

void *write_to_buffer();
void *read_by_data_mgr();
void *read_by_storage_mgr();
void fifo_writer_end(char* file_path,char* send_buffer);
//void fifo_writer(char* file_path,char* send_buffer);

int main(int agrc,char *argv[])
{
	if(agrc!=2)
	{
		printf("Please enter server port\n");
		exit(EXIT_FAILURE);
	}
	
	port=atoi(argv[1]);

	//counter is used to record the amount of valid data in data buffer
	
	

	int mkfifo_result=mkfifo(FIFO_NAME,0666);
	CHECK_MKFIFO(mkfifo_result); 

	pid_t child_pid;
	child_pid=fork();
	FORK_ERROR(child_pid);

	if(child_pid==0)
	{
		int sequence_nr=1;
		FILE * read_fifo=fopen(FIFO_NAME,"r");
		FILE_OPEN_ERROR(read_fifo);

		FILE * log_file=fopen(LOG_FILE_NAME,"w");
		FILE_OPEN_ERROR(log_file);

		char* end_indicator="END\n";
		char recv_buffer[MAX];
		char* recv_result;
		while(1)
		{
			recv_result=fgets(recv_buffer,MAX,read_fifo);
			if(recv_result!=NULL)
			{
				printf("Message received: %s", recv_buffer); 
				char* log_msg;
				asprintf(&log_msg,"<%d> %s",sequence_nr,recv_buffer);
				fputs(log_msg,log_file);
				free(log_msg);
				sequence_nr++;
			}
			if(strcmp(recv_buffer,end_indicator)==0)
			{
				fclose(read_fifo);
				fclose(log_file);
				break;
			}
		}
		
	}
	else
	{
		FILE* fp_sensor_map=fopen("room_sensor.map","r");
		insert_sensor_map(&datamgr,fp_sensor_map);

		counter=malloc(sizeof(int));
		*counter=0;

		pthread_mutex_init(&mutex_lock,NULL);
		sbuffer_init(&buffer);
		if(buffer==NULL) exit(EXIT_FAILURE);
		pthread_t writer,reader_data_mgr,reader_storage_mgr;
		
		
		pthread_create(&writer,NULL,write_to_buffer,NULL);
		pthread_create(&reader_data_mgr,NULL,read_by_data_mgr,NULL);
		pthread_create(&reader_storage_mgr,NULL,read_by_storage_mgr,NULL);

		pthread_join(writer,NULL);
		writer_finish_flag=1;
		//writer_finish_flag is used to indicate the end of writing process

		pthread_join(reader_data_mgr,NULL);
		fclose(fp_sensor_map);
		datamgr_free(datamgr);

		pthread_join(reader_storage_mgr,NULL);
		
		//write END into fifo to notify the reader to stop reading
		fifo_writer_end(FIFO_NAME,"END\n");
		
		pthread_mutex_destroy(&mutex_lock);
		
		free(counter);
		counter=NULL;
		sbuffer_free(&buffer);
		
	}
	
	return 0;
}



void fifo_writer_end(char* file_path,char* send_buffer)
{
	FILE * writer_fifo=fopen(file_path,"w");
	FILE_OPEN_ERROR(writer_fifo);
	
	if ( fputs( send_buffer, writer_fifo ) == EOF )
    	{
     	 	fprintf( stderr, "Error writing data to fifo\n");
      	 	exit( EXIT_FAILURE );
    	} 
	FFLUSH_ERROR(fflush(writer_fifo));
	int result=fclose(writer_fifo);
	FILE_CLOSE_ERROR(result);
	
}



void *write_to_buffer()
{
	connmgr_listen(port,buffer,counter,&mutex_lock,FIFO_NAME);
	return NULL;
}

void *read_by_data_mgr()
{
	int data_counter=0;
	sensor_data_t* data=NULL;
	if(buffer==NULL) exit(EXIT_FAILURE);

	while((data_counter<*counter)||writer_finish_flag==0)
	{
		pthread_mutex_lock(&mutex_lock);
		int size=sbuffer_size(buffer);
		for(int i=0;i<size;i++)
		{
			data=NULL;
			data=sbuffer_read_by_index(buffer,DATA_MGR,0);
			if(data!=NULL)
			{
				datamgr_parse_sensor_data(datamgr,data,FIFO_NAME);
				data_counter++;
			}
		}
		sbuffer_remove_processed_node(buffer);
		pthread_mutex_unlock(&mutex_lock);
	}
	
	return NULL;
}

void *read_by_storage_mgr()
{
	int try_count=0;
	DBCONN* db=NULL;
	time_t now;
	while(db==NULL)
	{
		db=init_connection(1);
		try_count++;
		if(db!=NULL)
		{
			char* send_buffer;
			asprintf(&send_buffer,"Connection to SQL server established\n");
			pthread_mutex_lock(&mutex_lock);
			fifo_writer(FIFO_NAME,send_buffer);
			pthread_mutex_unlock(&mutex_lock);
			free(send_buffer);
			
			break;
		}

		//wait TIME_WAIT second that is defined before
		now=time(NULL);
		while((time(NULL)-now)<TIME_WAIT);

		if(try_count==3&&db==NULL)
		{
			char* send_buffer;
			asprintf(&send_buffer,"Unable to connect to SQL server\n");
			pthread_mutex_lock(&mutex_lock);
			fifo_writer(FIFO_NAME,send_buffer);
			pthread_mutex_unlock(&mutex_lock);
			free(send_buffer);

			perror("Error connecting database");
			exit(EXIT_FAILURE);
		}
	}
	//try to connect the database at most three times

	int storage_counter=0;
	sensor_data_t* data=NULL;
	
	while((storage_counter<*counter)||writer_finish_flag==0)
	{
		
		pthread_mutex_lock(&mutex_lock);
		//sbuffer_remove_invalid_data_with_invalid_ID(buffer,datamgr);
		int size=sbuffer_size(buffer);
		for(int i=0;i<size;i++)
		{
			data=NULL;
			data=sbuffer_read_by_index(buffer,STORAGE_MGR,i);
			if(data!=NULL)
			{
				insert_sensor(db,data->id,data->value,data->ts);
				storage_counter++;
			}
		}
		sbuffer_remove_processed_node(buffer);
		pthread_mutex_unlock(&mutex_lock);
	}
	//find_sensor_all(db,read_data);
	disconnect(db);
	return NULL;
}

