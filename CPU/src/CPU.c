#include "CPU.h"

void log_protegido_cpu(char* mensaje){
	sem_wait(&mlog);
	log_info(logger_cpu, "%s", mensaje);
	sem_post(&mlog);
	free(mensaje);
}

/* -------------------------------------Iniciar CPU --------------------------------------------------*/

int main(int argc, char **argv){
	config_cpu = iniciar_config(argv[1]);
	logger_cpu = iniciar_logger("cpu.log","CPU");

	sem_init(&mlog,0,1);
	// pthread_t t3;
	pthread_t t1,t3, t4;
    pthread_create(&t1, NULL, (void*) conectarMemoria, NULL);   
    // pthread_create(&t2, NULL, (void*) conectarKernel, NULL);
	pthread_create(&t3, NULL, (void*) conectarKernelDispatch, NULL);
	pthread_create(&t4, NULL, (void*) conectarKernelInterrupt, NULL);

	pthread_join(t1, NULL);   
    // pthread_join(t2, NULL);
	pthread_join(t3, NULL);   
    pthread_join(t4, NULL);
	log_destroy(logger_cpu);
    config_destroy(config_cpu);

    return 0;
}

/* ------------------------------------Conexion Kernel -----------------------------------------------*/

int conectarKernelDispatch(){
	char* puerto_dispatch = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_DISPATCH");
	log_protegido_cpu(string_from_format("Leí el puerto %s\n",puerto_dispatch));

	socket_servidor_dispatch = iniciar_servidor(puerto_dispatch);
	if (socket_servidor_dispatch == -1 )
    {
        log_error(logger_cpu, "ERROR - No se pudo crear el servidor");
        return EXIT_FAILURE;
    }
    log_protegido_cpu(string_from_format("Servidor listo para recibir clientes\n"));
	socket_kernel_dispatch = esperar_cliente(socket_servidor_dispatch);
	int cod_op_handshake = recibir_operacion(socket_kernel_dispatch);

	if(cod_op_handshake == KERNEL_DISPATCH){
	log_protegido_cpu(string_from_format("Enviando mensaje de confirmacion..."));
    bool confirmacion = true;
    send(socket_kernel_dispatch, &confirmacion, sizeof(bool), 0);
    log_protegido_cpu(string_from_format("Mensaje enviado"));
	}
	else{
        log_error(logger_cpu, "ERROR en el handshake");
        return EXIT_FAILURE;
	}

    //t_buffer *buffer = crear_buffer();
	while (1)
	{
		// printf("esperando operacion\n");
		int cod_op = recibir_operacion(socket_kernel_dispatch);
		switch (cod_op)
		{
		case MENSAJE:
			recibir_mensaje(socket_kernel_dispatch, logger_cpu);
			break;
		case PCB:
			recibir_contexto_ejecucion(socket_kernel_dispatch);
			ejecutando_un_proceso = true;
			
			/*CREAR FUNCIÓN/HILO PARA EJECUTAR EL PROCESO(fetch,decode,execute)
			para que ejecute más de una instruccion*/
			//printf("Recibe un contexto, empieza a ejecutar\n");
			ejecutar_proceso();
			break;
		case -1:
			sem_wait(&mlog);
			log_error(logger_cpu, "El Kernel Dispatch se desconecto. Terminando servidor");
			sem_post(&mlog);
			//log_destroy(logger_cpu); // enviar error
			return EXIT_FAILURE;
		default:
			sem_wait(&mlog);
			log_warning(logger_cpu, "Kernel Dispatch: Operacion desconocida.");
			sem_post(&mlog);

			break;
		}
	}
	return EXIT_SUCCESS;
}

int conectarKernelInterrupt(){
	int socket_servidor_interrupt,socket_kernel_interrupt;
	char* puerto_interrupt = config_get_string_value(config_cpu, "PUERTO_ESCUCHA_INTERRUPT");
	log_protegido_cpu(string_from_format("Lei el puerto %s\n",puerto_interrupt));

	socket_servidor_interrupt = iniciar_servidor(puerto_interrupt);
	if (socket_servidor_interrupt == -1 )
    {
        log_error(logger_cpu, "ERROR - No se pudo crear el servidor");
        return EXIT_FAILURE;
    }
   log_protegido_cpu(string_from_format("Servidor listo para recibir clientes\n"));
	socket_kernel_interrupt = esperar_cliente(socket_servidor_interrupt);
	int cod_op_handshake = recibir_operacion(socket_kernel_interrupt);

	if(cod_op_handshake == KERNEL_INTERRUPT){
	log_protegido_cpu(string_from_format("Enviando mensaje de confirmacion..."));
    bool confirmacion = true;
    send(socket_kernel_interrupt, &confirmacion, sizeof(bool), 0);
   log_protegido_cpu(string_from_format("Mensaje enviado"));
	}
	else{
		 {
        log_error(logger_cpu, "ERROR en el handshake");
        return EXIT_FAILURE;
    }
	}

	log_protegido_cpu(string_from_format("Enviando mensaje de confirmacion..."));
    bool confirmacion = true;
    send(socket_kernel_interrupt, &confirmacion, sizeof(bool), 0);
    log_protegido_cpu(string_from_format("Mensaje enviado"));

    //t_buffer *buffer = crear_buffer();
	while (1)
	{
		int cod_op = recibir_operacion(socket_kernel_interrupt);
		switch (cod_op){
		case MENSAJE:
			recibir_mensaje(socket_kernel_interrupt,logger_cpu);
			break;
		case DESALOJAR_PROCESO:
			int size=0;
			void *buffer = recibir_buffer(&size, socket_kernel_interrupt);
			memcpy(&(motivo_interrupt), buffer, sizeof(int));
			memcpy(&(pid_interrupt), buffer + sizeof(int), sizeof(int));
			if(pid_interrupt==pid){
				interrupcion=true;
			}
			free(buffer);
			break;
		case -1:
			sem_wait(&mlog);
			log_error(logger_cpu, "El Kernel Interrupt se desconecto. Terminando servidor");
			sem_post(&mlog);
			return EXIT_FAILURE;
		default:
			sem_wait(&mlog);
			log_warning(logger_cpu, "Kernel Interrupt: Operacion desconocida.");
			sem_post(&mlog);
			break;
		}
	}
	return 0;
}

/* ------------------------------------Conexion Memoria --------------------------------------------*/

void conectarMemoria(){
	socket_memoria = -1;

	// Salva info config
    char* ip= config_get_string_value(config_cpu,"IP_MEMORIA");
	char* puerto= config_get_string_value(config_cpu,"PUERTO_MEMORIA");

    // log_protegido_cpu(string_from_format("CONEXION MEMORIA: Lei la IP %s y  puerto %s\n",ip,puerto));

    if((socket_memoria = crear_conexion(ip, puerto)) <= 0){
        log_error(logger_cpu, "No se ha podido conectar con la MEMORIA");
    }
	else{
    	int identificador = CPU;

		send(socket_memoria, &identificador, sizeof(int), 0);
		recv(socket_memoria, &tam_pag, sizeof(int), MSG_WAITALL);

		log_protegido_cpu(string_from_format("Conexion con memoria exitosa"));
		log_protegido_cpu(string_from_format("TAMANIO PAGINA MEMORIA: %d",tam_pag));
	}

}

/*-----------------------------------------------------------------------*/

void ejecutar_proceso(){
	while(ejecutando_un_proceso){
		check_interrupt(execute(decode(fetch())));
	}
}

void check_interrupt(instruccion* inst){
	if(interrupcion && ejecutando_un_proceso){
			devolver_contexto_de_ejecucion(motivo_interrupt,inst);
			ejecutando_un_proceso=false;
	}
	interrupcion = false;
	for (int i = 0; i < list_size(inst->parametros); i++)
    {
        char *parametro = list_get(inst->parametros, i);
        free(parametro);
    }
	list_destroy(inst->parametros);
	free(inst);
}

void recibir_contexto_ejecucion(int socket)
{
	int size=0;
	void *buffer = recibir_buffer(&size, socket);

	desempaquetar_contexto_ejecucion(buffer);
	free(buffer);
}

void desempaquetar_contexto_ejecucion(void *buffer)
{ // cpu
    //int size;
    //buffer = recibir_buffer(&size, socket_kernel_dispatch);
	// t_pcb* pcb = malloc(sizeof(t_pcb));
    int desplazamiento = 0;

	memcpy(&(pid), buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(&(program_counter), buffer + desplazamiento, sizeof(int));
	desplazamiento += sizeof(int);
	memcpy(&(registros_cpu.AX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&(registros_cpu.BX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&(registros_cpu.CX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
	memcpy(&(registros_cpu.DX), buffer + desplazamiento, sizeof(uint32_t));
	desplazamiento += sizeof(uint32_t);
}
