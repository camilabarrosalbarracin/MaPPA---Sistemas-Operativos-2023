#include "Kernel.h"

pthread_mutex_t mlog, mconexiones;

int conexiones;
int id_counter = 1;//PID DE LOS PROCESOS

/* -------------------------------------Iniciar Kernel -----------------------------------------------*/
int main(int argc, char **argv) {
	sem_init(&planificadores,0,0);
	config_kernel = iniciar_config(argv[1]);
	logger_kernel=iniciar_logger("kernel.log","KERNEL");
	pthread_mutex_init(&mconexiones,NULL);
	conexiones=0;

	pthread_t t1,t2,t3,t4,t5;
	inicializar_estructuras();

    pthread_create(&t1, NULL, (void*) conectarMemoria, NULL);
	pthread_create(&t2, NULL, (void*) conectarCpuDispatch, NULL);
	pthread_create(&t3, NULL, (void*) conectarCpuInterrupt, NULL);
    pthread_create(&t4, NULL, (void*) conectarFS, NULL);
	pthread_create(&t5, NULL, (void*) leer_consola, NULL);

	iniciar_hilos_estructuras();
	pthread_join(t5, NULL);

	free_estructuras();

	log_destroy(logger_kernel);
 	config_destroy(config_kernel);
	
	liberar_conexion(socket_memoria);
	liberar_conexion(socket_dispatch);
	liberar_conexion(socket_interrupt);
	liberar_conexion(socket_FS);

	pthread_detach(t1);
	pthread_detach(t2);
    pthread_detach(t3);
	pthread_detach(t4);
	sem_destroy(&planificadores);
    return 0;
}

/* ------------------------------------Conexion CPU --------------------------------------------------*/
int conectarCpuDispatch(){
	char* ip;
	char* puerto_dispatch;

	ip= config_get_string_value(config_kernel,"IP_CPU");
	puerto_dispatch=config_get_string_value(config_kernel,"PUERTO_CPU_DISPATCH");

    // log_protegido(string_from_format("CPU DISPATCH: Lei la IP %s y  puerto %s\n",ip,puerto_dispatch));
	
    socket_dispatch = crear_conexion(ip, puerto_dispatch);

    if (socket_dispatch <= 0){
        printf(" DISPATCH: No se pudo establecer una conexion con la CPU\n");
    }
    else{
		log_protegido(string_from_format("DISPATCH: Conexion con CPU exitosa"));
    }

	int handshake_dispatch = KERNEL_DISPATCH;
	bool confirmacion;
	send(socket_dispatch, &handshake_dispatch, sizeof(int),0); // MURIO ACA-----------------------------------------------------------------------
	recv(socket_dispatch, &confirmacion, sizeof(bool), MSG_WAITALL);
	
	if(confirmacion)
		log_protegido(string_from_format("Conexion de Modulo Dispatch con CPU exitosa"));
	else
		log_protegido(string_from_format("ERROR: Handshake de Modulo Dispatch con CPU fallido"));

	enviar_mensaje("SOY KERNEL DISPATCH",socket_dispatch);
	// printf("%d\n", socket_dispatch);
	pthread_mutex_lock(&mconexiones);
	conexiones++;
	pthread_mutex_unlock(&mconexiones);
	
return 0;
}


int conectarCpuInterrupt(){
	char* ip;
	char* puerto_interrupt;

	ip= config_get_string_value(config_kernel,"IP_CPU");
	puerto_interrupt= config_get_string_value(config_kernel,"PUERTO_CPU_INTERRUPT");

	// log_protegido(string_from_format("CPU INTERRUPT: Lei la IP %s y  puerto %s\n",ip,puerto_interrupt)); 
	
    socket_interrupt = crear_conexion(ip, puerto_interrupt);

	if (socket_interrupt <= 0){
        printf("INTERRUPT: No se pudo establecer una conexion con la CPU\n");
    }
    else{
		log_protegido(string_from_format("INTERRUPT: Conexion con CPU exitosa"));
		
    }

	int handshake_interrupt = KERNEL_INTERRUPT;
	bool confirmacion;
	send(socket_interrupt, &handshake_interrupt, sizeof(int),0);
	recv(socket_interrupt, &confirmacion, sizeof(bool), MSG_WAITALL);
	
	if(confirmacion)
		log_protegido(string_from_format("Conexion de Modulo Interrupt con CPU exitosa"));
	else
		log_protegido(string_from_format("ERROR: Handshake de Modulo Interrupt con CPU fallido"));

	enviar_mensaje("SOY KERNEL INTERRUPT",socket_interrupt);
	pthread_mutex_lock(&mconexiones);
	conexiones++;
	pthread_mutex_unlock(&mconexiones);
	return 0;
}


/* ------------------------------------Conexion Mermoria --------------------------------------------*/
int conectarMemoria(){
	socket_memoria = -1;
	char* ip;
	char* puerto;

    ip= config_get_string_value(config_kernel,"IP_MEMORIA");
	puerto=config_get_string_value(config_kernel,"PUERTO_MEMORIA");
	log_protegido(string_from_format("MEMORIA:Conectando a memoria..."));

	while((socket_memoria = crear_conexion(ip, puerto)) <= 0){
		log_protegido(string_from_format("No se pudo establecer una conexion con la Memoria"));
        sleep(1);
	}

	int handshake = KERNEL;
	bool confirmacion;

	send(socket_memoria, &handshake, sizeof(int),0);		//no hace handshake
	recv(socket_memoria, &confirmacion, sizeof(bool), MSG_WAITALL);

	
	if(confirmacion)
		log_protegido(string_from_format("Conexion con memoria exitosa"));
	else
		log_protegido(string_from_format("ERROR: Handshake con memoria fallido"));

	pthread_mutex_lock(&mconexiones);
	conexiones++;
	pthread_mutex_unlock(&mconexiones);

return 0;

}

/* ------------------------------------Conexion FS -------------------------------------------------*/
int conectarFS(){
    
	char* ip;
	char* puerto;

	ip= config_get_string_value(config_kernel,"IP_FILESYSTEM");
	puerto=config_get_string_value(config_kernel,"PUERTO_FILESYSTEM");

    // log_protegido(string_from_format("Lei la IP %s y  puerto %s del FS\n",ip,puerto));

    socket_FS = crear_conexion(ip, puerto);

    if (socket_FS<=0){
        printf("No se pudo establecer una conexion con FILE SYSTEM \n");
    }
	
	int handshake = KERNEL;
	bool confirmacion=false;
	send(socket_FS, &handshake, sizeof(int),0);
	recv(socket_FS, &confirmacion, sizeof(bool), MSG_WAITALL);
	
	if(confirmacion)
		log_protegido(string_from_format("Conexion con FILE SYSTEM exitosa"));
	else
		log_protegido(string_from_format("ERROR: Handshake con FILE SYSTEM fallido"));

	pthread_mutex_lock(&mconexiones);
	conexiones++;
	pthread_mutex_unlock(&mconexiones);

	return 0;
}

void leer_consola(){
		while(conexiones<4){/*ESPERA*/}
		char* leido;
		leido = readline(">");

		while(strcmp(leido, "EXIT") != 0)
		{			
			t_comando* comando;
			comando = malloc(sizeof(t_comando));
			comando->parametros=list_create();
			interpretar(comando,leido);
			//printf("el codigo es: %d\n",comando->cod_op);
			switch (comando->cod_op)
			{
			case INICIAR_PLANIFICACION:
				reanudar_ejecucion();
				sem_post(&planificadores);
				break;
			case DETENER_PLANIFICACION:
				pausar_ejecucion();
				break;
			case MULTIPROGRAMACION:
				int nuevo_grado_mult = atoi(list_get(comando->parametros,0));
				int grado_multiprogramacion_viejo=grado_multiprogramacion;
				cambiar_multiprogramacion(nuevo_grado_mult);
				log_protegido(string_from_format("Grado Anterior: <%d> - Grado Actual: <%d>",grado_multiprogramacion_viejo,nuevo_grado_mult));

				break;
			case INICIAR_PROCESO:
				/*Ante la solicitud de la consola de crear un nuevo proceso el Kernel debera informarle a 
				la memoria que debe crear un proceso con el nombre del archivo de pseudocodigo
				junto con el tamanio En bytes que ocupara el mismo.*/
				t_pcb *pcb = generar_pcb(comando);
				t_paquete* proceso = crear_paquete(INICIAR);
				char* size_char = list_get(comando->parametros,1);
				double size = strtod(size_char, NULL);
				char* path = list_get(comando->parametros,0);
				//char* path_inicio="/home/utnso/Documents/tp-2023-2c-9-30NoMeLevanto/Kernel/PRUEBAS/"; //compu Mateo
				char* path_inicio="/home/utnso/tp-2023-2c-9-30NoMeLevanto/Kernel/PRUEBAS/"; //compu Avril
				//char* path_inicio="/home/utnso/Documents/TP Operativos/tp-2023-2c-9-30NoMeLevanto/Kernel/";//compu Bauti
				char* path_fin=".txt";
				char* path_completo= (char *)malloc(strlen(path_inicio)+ strlen(path)+strlen(path_fin) + 1);
				strcpy(path_completo, path_inicio);
				strcat(path_completo,path);
				strcat(path_completo,path_fin);
				//printf("PATH: %s \n", path_completo);
				agregar_a_paquete_solo(proceso,&(pcb->id),sizeof(int));
				agregar_a_paquete_solo(proceso,&(size),sizeof(double));
				agregar_a_paquete(proceso,path_completo,strlen(path_completo)+1);
				enviar_paquete(proceso, socket_memoria);
				bool confirmacion;
				recv(socket_memoria, &confirmacion, sizeof(bool), MSG_WAITALL); 
				if(!confirmacion){
					log_error(logger_kernel,"NO se pudo crear la tabla de páginas del proceso.");
					break;
				}
				agregar_a_new(pcb);
				free(path_completo);
				eliminar_paquete(proceso);
				break;
			case FINALIZAR_PROCESO:
				/* removemos el proceso de pid enviado por consola y lo agregamos a exit
				En caso de que el proceso se encuentre ejecutando en CPU, se deberá enviar una 
				señal de interrupción a través de la conexión de interrupt con el mismo y 
				aguardar a que éste retorne el Contexto de Ejecución antes de iniciar la
				liberación de recursos. 
				*/
				int pid=atoi(list_get(comando->parametros,0));
				printf("finalizar el proceso de pid %d\n",pid);
				if(proceso_exec != NULL){
					if (proceso_exec->id==pid)
					{
						//pausar_ejecucion();//dentro se espera el contexto
						if (!volvio_pcb_cpu)
						{
							desalojar_proceso(FINALIZAR_PROCESO);
						}
						else {
							finalizar_proceso(proceso_exec,FINALIZAR_PROCESO);
							proceso_exec=NULL;
							}
					}
					else{
						buscar_y_finalizar_proceso(pid,FINALIZAR_PROCESO);
					}
				}
				else{
					buscar_y_finalizar_proceso(pid,FINALIZAR_PROCESO);
				}
				//if(deadlock)detectar_deadlock();
				break;
			case PROCESO_ESTADO:
				listar_procesos_por_estado();
				break;
			
			default:
				break;
			}
			free(leido);
			if (comando->parametros!=NULL)
			{	
				for (int i = 0; i < list_size(comando->parametros); i++)
				{
					free(list_get(comando->parametros,i));
				}
			}
			list_destroy(comando->parametros);
			free(comando);
			leido = readline(">");
		}
		free(leido);
		
		pthread_exit(NULL);
}

/*
INICIAR_PROCESO [PATH] [SIZE] [PRIORIDAD]

Finalizar proceso: Se encargará de finalizar un proceso que se encuentre dentro del sistema.
 Este mensaje se encargará de realizar las mismas operaciones como si el proceso llegara a EXIT 
 por sus caminos habituales (deberá liberar recursos, archivos y memoria).
Nomenclatura: FINALIZAR_PROCESO [PID]

Detener planificación: Este mensaje se encargará de pausar la planificación de corto y largo plazo. 
El proceso que se encuentra en ejecución NO es desalojado, pero una vez que salga de EXEC se va a 
pausar su transición al siguiente estado.
Nomenclatura: DETENER_PLANIFICACION

Iniciar planificación: Este mensaje se encargará de retomar (en caso que se encuentre pausada)
 la planificación de corto y largo plazo. En caso que la planificación no se encuentre pausada, 
 se debe ignorar el mensaje.
Nomenclatura: INICIAR_PLANIFICACION

Modificar grado multiprogramación: Permitirá la actualización del grado de multiprogramación 
configurado inicialmente por archivo de configuración. En caso que dicho valor sea inferior 
al actual NO se debe desalojar ni finalizar los procesos.
Nomenclatura: MULTIPROGRAMACION [VALOR]

Listar procesos por estado: Se encargará de mostrar por consola el listado de los estados con 
los procesos que se encuentran dentro de cada uno de ellos.
Nomenclatura: PROCESO_ESTADO

*/

void interpretar(t_comando* comando, char* leido){
	char**leido_separado = string_split(leido," ");
	comando->cod_op=500;
	if(!strcmp(leido_separado[0],"INICIAR_PLANIFICACION")){//sin parametros
		comando->cod_op=INICIAR_PLANIFICACION;
		//comando->parametros =NULL;
	}
	if(!strcmp(leido_separado[0],"DETENER_PLANIFICACION"))//sin parametros
	{
		comando->cod_op=DETENER_PLANIFICACION;
		//comando->parametros =NULL;
	}
	if(!strcmp(leido_separado[0],"MULTIPROGRAMACION"))//un parametro, VALOR
	{
		comando->cod_op=MULTIPROGRAMACION;
		char* p1= malloc(strlen(leido_separado[1])+1);
		strcpy(p1,leido_separado[1]);
		list_add(comando->parametros, p1);
	}
	if(!strcmp(leido_separado[0],"INICIAR_PROCESO"))//tres parametros, PATH SIZE PRIORIDAD
	{
		comando->cod_op=INICIAR_PROCESO;
		char* p1= malloc(strlen(leido_separado[1])+1);
		strcpy(p1,leido_separado[1]);
		list_add(comando->parametros, p1);
		//printf("%s\n",leido_separado[1]);
		char* p2= malloc(strlen(leido_separado[2])+1);
		strcpy(p2,leido_separado[2]);
		list_add(comando->parametros, p2);
		char* p3= malloc(strlen(leido_separado[3])+1);
		strcpy(p3,leido_separado[3]);
		list_add(comando->parametros, p3);
	}
	if(!strcmp(leido_separado[0],"FINALIZAR_PROCESO"))//un parametro PID
	{
		comando->cod_op=FINALIZAR_PROCESO;
		char* p1= malloc(strlen(leido_separado[1])+1);
		strcpy(p1,leido_separado[1]);
		list_add(comando->parametros, p1);
	}
	if(!strcmp(leido_separado[0],"PROCESO_ESTADO"))//sin parametros
	{
		comando->cod_op=PROCESO_ESTADO;
		//comando->parametros =NULL;
	}
	int i = 0;
	while (leido_separado[i] != NULL) {
		free(leido_separado[i]);
		i++;
	}
	free(leido_separado);
}

t_pcb *generar_pcb(t_comando* comando) {
	t_pcb *pcb = (t_pcb *) malloc(sizeof(t_pcb)); 
	pcb->id = id_counter;
	char* prioridad_char = list_get(comando->parametros,2);
	int prioridad = atoi(prioridad_char);
	pcb->prioridad=prioridad;
	pcb->program_counter = 0;
	inicializar_registros(&(pcb->registros_cpu));

	pcb->recursos_asignados=list_create();
	pcb->archivos_abiertos=list_create();
	id_counter++;
	pcb->tiempo_llegada = temporal_create();
	temporal_gettime(pcb->tiempo_llegada); //para desempatar prioridades
	return pcb;
}
