#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "config.h"
#include <sqlite3.h>
#include "sensor_db.h"


int show_error_msg(char* error_msg);
int read_data(void *, int, char **, char **);

DBCONN * init_connection(char clear_up_flag) {
    DBCONN *db=NULL;
    char *dbName = TO_STRING(DB_NAME);
    char *error_msg = NULL;
    int rs = sqlite3_open(dbName, &db);

    if (rs == SQLITE_ERROR)
    {
        printf("%s\n",error_msg);
        return NULL;
    }
    else if(clear_up_flag==1) {
        const char* dropTable_sql="DROP TABLE IF EXISTS "TO_STRING(TABLE_NAME)";";
        //drop the original table if the flag is set
        sqlite3_exec(db,dropTable_sql,NULL,NULL,&error_msg);
    }

    const char* createTable_sql="CREATE TABLE IF NOT EXISTS "TO_STRING(TABLE_NAME)"(sensor_id INT, sensor_value DECIMAL(4,2), timestamp TIMESTAMP);";
    sqlite3_exec(db,createTable_sql,NULL,NULL,&error_msg);
    //create new table if the table is not existed. Or do not make change

    if(error_msg!=NULL)
        printf("%s\n",error_msg);
    return db;
}

int insert_sensor(DBCONN * conn, sensor_id_t id, sensor_value_t value, sensor_ts_t ts)
{
    char* error_msg=NULL;
    char* insertData_sql=malloc(100*sizeof(char));
    sprintf(insertData_sql,"INSERT INTO "TO_STRING(TABLE_NAME)"(sensor_id, sensor_value, timestamp) VALUES(%hu,%f,%ld)",id,value,ts);
    //use sprintf function to parsing parameter into sql sentence

    sqlite3_exec(conn,insertData_sql,NULL,NULL,&error_msg);
    free(insertData_sql);
    return show_error_msg(error_msg);
}

int insert_sensor_from_file(DBCONN * conn, FILE * sensor_data)
{
    sensor_id_t id=0;
    sensor_value_t data=0;
    sensor_ts_t timestamp=0;
    while(fread(&id, sizeof(sensor_id_t),1,sensor_data)!=0&&fread(&data, sizeof(double), 1, sensor_data)!=0
          &&fread(&timestamp, sizeof(sensor_ts_t), 1, sensor_data)!=0)
    {
        int result=insert_sensor(conn,id,data,timestamp);
        if(result!=0)
            return -1;
    }
    return 0;
}

int find_sensor_all(DBCONN * conn, callback_t f)
{
    char* error_msg=NULL;
    const char* select_sql="SELECT * FROM "TO_STRING(TABLE_NAME)"";
    sqlite3_exec(conn,select_sql,f,NULL,&error_msg);
    return show_error_msg(error_msg);
}

int find_sensor_by_value(DBCONN * conn, sensor_value_t value, callback_t f)
{
    char* error_msg=NULL;
    char* findByValue_sql=malloc(100* sizeof(char));
    sprintf(findByValue_sql,"SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE sensor_value=%f",value);
    printf("%s\n",findByValue_sql);
    sqlite3_exec(conn,findByValue_sql,f,NULL,&error_msg);
    free(findByValue_sql);
    return show_error_msg(error_msg);
}
//case: no such a value

int find_sensor_exceed_value(DBCONN * conn, sensor_value_t value, callback_t f)
{
    char* error_msg=NULL;
    char* findExceedValue_sql=malloc(100* sizeof(char));
    sprintf(findExceedValue_sql,"SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE sensor_value>%f",value);
    printf("%s\n",findExceedValue_sql);
    sqlite3_exec(conn,findExceedValue_sql,f,NULL,&error_msg);
    free(findExceedValue_sql);
    return show_error_msg(error_msg);
}

int find_sensor_by_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
    char* error_msg=NULL;
    char* findByTimestamp_sql=malloc(100* sizeof(char));
    sprintf(findByTimestamp_sql,"SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE timestamp=%ld",ts);
    printf("%s\n",findByTimestamp_sql);
    sqlite3_exec(conn,findByTimestamp_sql,f,NULL,&error_msg);
    free(findByTimestamp_sql);
    return show_error_msg(error_msg);
}

int find_sensor_after_timestamp(DBCONN * conn, sensor_ts_t ts, callback_t f)
{
    char* error_msg=NULL;
    char* findAfterTimestamp_sql=malloc(100* sizeof(char));
    sprintf(findAfterTimestamp_sql,"SELECT * FROM "TO_STRING(TABLE_NAME)" WHERE timestamp>%ld",ts);
    printf("%s\n",findAfterTimestamp_sql);
    sqlite3_exec(conn,findAfterTimestamp_sql,f,NULL,&error_msg);
    free(findAfterTimestamp_sql);
    return show_error_msg(error_msg);
}

void disconnect(DBCONN *conn)
{
    sqlite3_close(conn);
    conn=NULL;
}

int read_data(void * NotUsed, int column_amount, char ** column_value, char ** column_name)
{
    NotUsed=NULL;
    for(int index=0;index<column_amount;index++)
    {
        printf("%s=%s\n",column_name[index],column_value[index]);
    }
    printf("\n");
    return 0;
}
//callback function is called every row

int show_error_msg(char* error_msg)
{
    if(error_msg==NULL)
        return 0;
    else
    {
        printf("%s\n",error_msg);
        return -1;
    }
}
