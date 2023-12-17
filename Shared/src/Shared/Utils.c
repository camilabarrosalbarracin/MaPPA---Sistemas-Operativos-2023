#include "Utils.h"

t_log* logger;

/*-------------------------------------------SERVIDOR----------------------------------------------------*/

int iniciar_servidor(char* puerto)
{
	// Quitar esta línea cuando hayamos terminado de implementar la funcion
//	assert(!"no implementado!");

	int socket_servidor;

	struct addrinfo hints, *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(NULL, puerto, &hints, &servinfo);

	// Creamos el socket de escucha del servidor
	socket_servidor= socket(servinfo->ai_family,
							servinfo->ai_socktype,
							servinfo->ai_protocol);

	// Asociamos el socket a un puerto
	bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
	// Escuchamos las conexiones entrantes
	listen(socket_servidor, SOMAXCONN);

	freeaddrinfo(servinfo);

	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	// Quitar esta línea cuando hayamos terminado de implementar la funcion
//	assert(!"no implementado!");

	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor, NULL, NULL);
	//log_info(logger, "Se conecto un cliente!");
	return socket_cliente;
}

int recibir_operacion(int socket_cliente){
	int cod_op=0;
	if(recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) > 0)
		return cod_op;
	else
	{
		close(socket_cliente);
		return -1;
	}
}

void* recibir_buffer(int* size, int socket_cliente){
	void * buffer;

	recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
	buffer = malloc(*size);
	recv(socket_cliente, buffer, *size, MSG_WAITALL);

	return buffer;
}

void recibir_mensaje(int socket_cliente, t_log* logger){
	int size=0;
	char* buffer = recibir_buffer(&size, socket_cliente);
	log_info(logger, "Me llego el mensaje %s\n", buffer);
	free(buffer);
}

t_list* recibir_paquete(int socket_cliente){
	int size=0;
	int desplazamiento = 0;
	void * buffer;
	t_list* valores = list_create();
	int tamanio;

	buffer = recibir_buffer(&size, socket_cliente);
	while(desplazamiento < size)
	{
		memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
		desplazamiento+=sizeof(int);
		char* valor = malloc(tamanio);
		memcpy(valor, buffer+desplazamiento, tamanio);
		desplazamiento+=tamanio;
		list_add(valores, valor);
	}
	free(buffer);
	return valores;
}

/*-------------------------------------------CLIENTE----------------------------------------------------*/

void* serializar_paquete(t_paquete* paquete, int bytes){
	void * magic = malloc(bytes);
	int desplazamiento = 0;

	memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
	desplazamiento+= sizeof(int);
	memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
	desplazamiento+= paquete->buffer->size;

	return magic;
}

int crear_conexion(char *ip, char* puerto){
	struct addrinfo hints;
	struct addrinfo *server_info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	getaddrinfo(ip, puerto, &hints, &server_info);

	// Ahora vamos a crear el socket.
	int socket_cliente = socket(server_info->ai_family,
            					server_info->ai_socktype,
								server_info->ai_protocol);

	// Ahora que tenemos el socket, vamos a conectarlo
	if(connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
	{
		printf("Error en la conexión");
		socket_cliente = -1;
	}

	freeaddrinfo(server_info); 
	return socket_cliente;
}

void enviar_mensaje(char* mensaje, int socket_cliente){
	t_paquete* paquete = malloc(sizeof(t_paquete));

	paquete->codigo_operacion = MENSAJE;
	paquete->buffer = malloc(sizeof(t_buffer));
	paquete->buffer->size = strlen(mensaje) + 1;
	paquete->buffer->stream = malloc(paquete->buffer->size);
	memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

	int bytes = paquete->buffer->size + 2 * sizeof(int); // buff	
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
	eliminar_paquete(paquete);
}


t_buffer *crear_buffer(){
	t_buffer *buffer = malloc(sizeof(t_buffer));
	assert(buffer != NULL);

	buffer->size = 0;
	buffer->stream = NULL;
	return buffer;
}

t_paquete *crear_paquete(int codigo_operacion){
	t_paquete *paquete = (t_paquete *)malloc(sizeof(t_paquete));

	assert(paquete != NULL);

	paquete->codigo_operacion = codigo_operacion;
	paquete->buffer = crear_buffer();
	return paquete;
}

void agregar_a_paquete(t_paquete* paquete, void* valor, int tamanio){
	paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

	memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
	memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), valor, tamanio);

	paquete->buffer->size += tamanio + sizeof(int);
}

void agregar_a_paquete_solo(t_paquete *paquete, void *valor, int bytes)
{
	t_buffer *buffer = paquete->buffer;
	buffer->stream = realloc(buffer->stream, buffer->size + bytes);
	memcpy(buffer->stream + buffer->size, valor, bytes);
	buffer->size += bytes;
}

void enviar_paquete(t_paquete* paquete, int socket_cliente){
	int bytes = paquete->buffer->size + 2*sizeof(int);
	void* a_enviar = serializar_paquete(paquete, bytes);

	send(socket_cliente, a_enviar, bytes, 0);

	free(a_enviar);
}

void eliminar_paquete(t_paquete* paquete){
	free(paquete->buffer->stream);
	free(paquete->buffer);
	free(paquete);
}

void liberar_conexion(int socket_cliente){
	close(socket_cliente);
}


u_int8_t get_cant_parametros(u_int8_t identificador)
{
	u_int8_t cant_parametros = 0;

	switch (identificador)
	{
	case EXIT:
		break;

	case SLEEP:
	case F_CLOSE:
	case WAIT:
	case SIGNAL:
	 	cant_parametros = 1;
		break;

	case SET:
	case SUM:
	case SUB:
	case MOV_IN:
	case MOV_OUT:
	case F_OPEN:
	case F_TRUNCATE:
	case F_SEEK:
	case F_WRITE:
	case F_READ:
	case JNZ:
		cant_parametros = 2;
		break;

	}
	return cant_parametros;
}

char *get_motivo(int motivo)
{
	char *identificadores[] = {
		"MENSAJE",
		"PAQUETE",
		"KERNEL_INTERRUPT",
		"KERNEL_DISPATCH",
		"KERNEL",
		"FS",
		"CPU",
		"MEMORIA",
		"CONTEXTO_EJECUCION",
		"INICIAR_PLANIFICACION",
		"DETENER_PLANIFICACION",
		"MULTIPROGRAMACION",
		"INICIAR_PROCESO",
		"FINALIZAR_PROCESO",
		"PROCESO_ESTADO",
		"NEW",
		"READY",
		"EXEC",
		"BLOCKED",
		"EXIT",
		"PCB",
		"SET",
		"SUM",
		"SUB",
		"MOV_IN",
		"MOV_OUT",
		"SLEEP",
		"JNZ",
		"WAIT",
		"SIGNAL",
		"F_CREATE",
		"F_OPEN",
		"F_TRUNCATE",
		"F_SEEK",
		"F_WRITE",
		"F_READ",
		"F_CLOSE",
		"INVALID_RESOURCE",
		"INVALID_WRITE",
		"SUCCESS",
		"PAUSAR_EJECUCION",
		"INICIAR",
		"PRIORIDADES"
	};
	return identificadores[motivo];
}