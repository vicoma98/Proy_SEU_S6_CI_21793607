#include "FreeRTOS.h"
#include <stdio.h>
#include "tareas_serie.h"
#include <task.h>
#include "cmsis_os.h"
#include "main.h"
#include "cJSON.h"
#include "utility.h"
#include <string.h>
#include <stm32f4xx_hal_dma.h>
#include "time.h"

void Task_Display( void *pvParameters );
void Task_DMA( void *pvParameters );
void Task_Send( void *pvParameters );
void Task_Receive( void *pvParameters );

#define buffer_DMA_size 2048
uint8_t buffer_DMA[buffer_DMA_size];
extern UART_HandleTypeDef huart1;
#define UART_ESP_AT_WIFI (&huart1)
#define UART_ESP8266 (&huart1)

char post_temp[]="POST  /v1/queryContext HTTP/1.1\r\nAccept: text/html\r\nHost: pperez-seu-or.disca.upv.es\r\nContent-Legth: %d\r\n\r\n%d";
char identificador[]="Sensor_SEU_S6_VCM06";
char body []="{\"entities\":[{\"type\":\"Sensor\",\"isPattern\":\"false\",\"id\":\"Sensor_SEU_S6_VCM06\"}]}";
char cadenafinalv2[1000];


extern BUFF_BUFFER_t * buff;
extern BUFF_BUFFER_t * buff_rx;

void serie_Init_FreeRTOS(void){

	BaseType_t res_task;

	printf (PASCU_PRJ " at "__TIME__);
	fflush(0);

	buff=bufferCreat(128);
	if (!buff) return;

	buff_rx=bufferCreat(512);
	if (!buff_rx) return;

	res_task=xTaskCreate(Task_Display,"DISPLAY",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea Visualizador\r\n");
			fflush(NULL);
			while(1);
	}

	res_task=xTaskCreate(Task_DMA,"DMA",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
		if( res_task != pdPASS ){
				printf("PANIC: Error al crear Tarea Visualizador\r\n");
				fflush(NULL);
				while(1);
		}

	res_task=xTaskCreate(Task_Send,"ENVIO",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea Visualizador\r\n");
			fflush(NULL);
			while(1);
	}

	res_task=xTaskCreate(Task_Receive,"RECEIVE",512,NULL,	makeFreeRtosPriority(osPriorityNormal),NULL);
	if( res_task != pdPASS ){
			printf("PANIC: Error al crear Tarea Visualizador\r\n");
			fflush(NULL);
			while(1);
	}
}

char candenafinal[100];
int funcion_conf(char * cadena[],int * len,int * osDelay_, int * osDelay_2){
	int vuelta=1;
	uint32_t res;
	res=HAL_UART_Transmit(UART_ESP_AT_WIFI,cadena,len,1000);
	HAL_UART_Receive_DMA(UART_ESP_AT_WIFI, buffer_DMA,buffer_DMA_size);
	osDelay(osDelay_);
	HAL_UART_DMAStop(UART_ESP_AT_WIFI);
	int buffer_ct1=buffer_DMA_size - HAL_DMA_getcounter(UART_ESP_AT_WIFI);
	int buffer_ct=0;
	while (buffer_ct<buffer_ct1)
		res=buff->put(buff,buffer_DMA[buffer_ct++]);
	osDelay(osDelay_2);

	return vuelta;
}

void entregable(char * nombreMaquina,char * ssid, char * passwd, char * puerto){
	uint32_t res;
		char cad[]="AT+CWJAP=\"%s\",\"%s\"\r\n";
		char cad1[]="AT+CWMODE=1\r\n";
		char cad2[]="AT+CIFSR\r\n";
		char cad3[]="AT+CIPSTART=\"TCP\",\"%s\",%s\r\n";
		sprintf(candenafinal,cad3,nombreMaquina,puerto);
		int r = funcion_conf(cad1,strlen(cad1),2000,1);
		sprintf(candenafinal,cad,ssid,passwd);
		int r2 = funcion_conf(candenafinal,strlen(candenafinal),2000,1);
		int r3 = funcion_conf(cad2,strlen(cad2),2000,1);
		sprintf(candenafinal,cad3,nombreMaquina,puerto);
		int r4 = funcion_conf(candenafinal,strlen(candenafinal),500,1000);
		//**********get************//
		char cad4[]="AT+CIPSEND=%d\r\n";
		char get[]="GET /api/json/cet/now HTTP/1.1\r\nAccept: text/html\r\nHost: worldclockapi.com\r\n\r\n";
		char cadenafinalv2[strlen(get)+strlen(cad4)];
		sprintf(cadenafinalv2,cad4,strlen(get));
		int r5 = funcion_conf(cadenafinalv2,strlen(get),500,2000);
		int r6 = funcion_conf(get,strlen(get),500,20);

		//tratado del json
		char *jsonp=strstr(buffer_DMA,"{");
		jsonp[strlen(jsonp)-2]='\0';
		//meter en struct para poder funcionar
		const cJSON *fecha = NULL;
		cJSON * jsonB = cJSON_Parse(jsonp);
		fecha = cJSON_GetObjectItemCaseSensitive(jsonB, "currentDateTime");
		char* fechaSTR= fecha->valuestring;
		struct tm hora;
		char *format = "%Y-%Om-%dT%H:%M%z";
		strptime(fechaSTR, format, &hora);
		//bucle de printeo
		while(1){
			hora.tm_sec+=2;
			mktime(&hora);
			char hora2[100];
			strftime(hora2,100,"%Y-%Om-%d %H:%M:%S\r\n",&hora);
			res=buff->puts(buff,(BUFF_ITEM_t *)hora2,strlen(hora2));
			vTaskDelay(2000/portTICK_RATE_MS );
		}
}
void postfunc(char * nombreMaquina,char * ssid, char * passwd, char * puerto){
	uint32_t res;
	char cad[]="AT+CWJAP=\"%s\",\"%s\"\r\n";
	char cad1[]="AT+CWMODE=1\r\n";
	char cad2[]="AT+CIFSR\r\n";
	char cad3[]="AT+CIPSTART=\"TCP\",\"%s\",%s\r\n";
	sprintf(candenafinal,cad3,nombreMaquina,puerto);
	int r = funcion_conf(cad1,strlen(cad1),2000,1);
	sprintf(candenafinal,cad,ssid,passwd);
	int r2 = funcion_conf(candenafinal,strlen(candenafinal),2000,1);
	int r3 = funcion_conf(cad2,strlen(cad2),2000,1);
	sprintf(candenafinal,cad3,nombreMaquina,puerto);
	int r4 = funcion_conf(candenafinal,strlen(candenafinal),500,1000);

		//***post***
		sprintf(candenafinal,post_temp,strlen(body),body);
		char cad4[]="AT+CIPSEND=%d\r\n";
		sprintf(cadenafinalv2,cad4,strlen(candenafinal));
		int r5 = funcion_conf(cadenafinalv2,strlen(cadenafinalv2),500,2000);



		HAL_UART_Receive_DMA(UART_ESP8266, buffer_DMA,2048);
		res=HAL_UART_Transmit(UART_ESP_AT_WIFI,candenafinal,strlen(candenafinal),1000);
		osDelay(500);
		HAL_UART_DMAStop(UART_ESP_AT_WIFI);
		int buffer_ct1=buffer_DMA_size - HAL_DMA_getcounter(UART_ESP_AT_WIFI);
		int buffer_ct=0;
		while (buffer_ct<buffer_ct1)
			res=buff->put(buff,buffer_DMA[buffer_ct++]);
		osDelay(20);
		//tratado del json
		/*char *jsonp=strstr(buffer_DMA,"{");
		jsonp[strlen(jsonp)-2]='\0';
		//meter en struct para poder funcionar
		const cJSON *fecha = NULL;
		cJSON * jsonB = cJSON_Parse(jsonp);
		fecha = cJSON_GetObjectItemCaseSensitive(jsonB, "currentDateTime");
		char* fechaSTR= fecha->valuestring;
		struct tm hora;
		char *format = "%Y-%Om-%dT%H:%M%z";
		strptime(fechaSTR, format, &hora);
		//bucle de printeo
		while(1){
			hora.tm_sec+=2;
			mktime(&hora);
			char hora2[100];
			strftime(hora2,100,"%Y-%Om-%d %H:%M:%S\r\n",&hora);
			res=buff->puts(buff,(BUFF_ITEM_t *)hora2,strlen(hora2));
			vTaskDelay(2000/portTICK_RATE_MS );
		}*/
}


void Task_Send( void *pvParameters ){

	uint32_t it;
    uint32_t res;

	char cad[100];
	it=0;
	//boot_wifi("OPPOReno2","ilovematy");
	//entregable("worldclockapi.com","OPPOReno2","ilovematy","80");
	//postfunc("pperez-seu-or.disca.upv.es","routerSEU","00000000","10000");
	postfunc("imaginye.ddns.net","OPPOReno2","ilovematy","1026");
	while(1){
		it++;
		sprintf(cad,"IT %d\r\n",(int)it);
		res=buff->puts(buff,(BUFF_ITEM_t *)cad,strlen(cad));
		vTaskDelay(10000/portTICK_RATE_MS );

	}
}

void Task_Receive( void *pvParameters ){

	uint32_t it;
    uint32_t res;
    BUFF_ITEM_t  car;
#define buffer_length	128
    BUFF_ITEM_t  buffer[buffer_length];
    int buffer_ct,buffer_ct1;
    int crln_detect;

	it=0;


	while(1){
		it++;

		crln_detect=0;
		buffer_ct=0;

		while(crln_detect<2){
	    	res=buff->get(buff_rx,&car);
	    	buffer[buffer_ct++]=car;
	    	if (buffer_ct>1){

	    		if ((buffer[buffer_ct-2]=='\r')&&(buffer[buffer_ct-1]=='\n')) // \r\n detection end of line
					crln_detect=2;
				else
					if ((buffer_ct)==buffer_length)  // line out of limits --> error
						crln_detect=3;
	    	}

		}

		// prepare reception buffer from ESP
		HAL_UART_Receive_DMA(UART_ESP_AT_WIFI, buffer_DMA,buffer_DMA_size);
		// send line (command) to ESP
		res=HAL_UART_Transmit(UART_ESP_AT_WIFI,buffer,buffer_ct,1000);
		// wait a bit time
		osDelay(500);
		//stop reception probably all data are in dma buffer
		HAL_UART_DMAStop(UART_ESP_AT_WIFI);

		// send to console ESP answer.
		buffer_ct1=HAL_DMA_getcounter(UART_ESP_AT_WIFI);
		buffer_ct=0;
		while (buffer_ct<buffer_ct1)
			res=buff->put(buff,buffer_DMA[buffer_ct++]);
		// wait a bit time
		osDelay(1);
	}
}
