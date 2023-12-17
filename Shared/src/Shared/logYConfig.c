#include "logYConfig.h"

t_log* iniciar_logger(char* nombreLog, char* proceso){

	t_log* nuevo_logger= log_create(nombreLog,proceso,1,LOG_LEVEL_INFO);

	if(nuevo_logger ==NULL){
		 printf("No pude crear el logger\n");}
		 // si se debe meter en la funcion agrego exit(1);

	return nuevo_logger;
}

t_config* iniciar_config(char* archivo){

	t_config* nuevo_config= config_create(archivo);

	if(nuevo_config ==NULL){
		printf("No pude leer la config \n");
		exit(2);
	}

	return nuevo_config;
}