#include "instrucciones.h"

extern t_registros_cpu registros_cpu;

//TO DO: FALTA CAMBIARLO Y PEDIRLE A MEMORIA LA INSTRUCCION PASANDOLE EL PROGRAM_COUNTER, recibir_mensaje
char *fetch(){
    log_info(logger_cpu,"PID: <%d> - FETCH - Program Counter: <%d>",pid,program_counter);
    t_paquete* paquete = crear_paquete(FETCH_INSTRUCCION);
    agregar_a_paquete_solo(paquete,&pid,sizeof(int));
    agregar_a_paquete_solo(paquete,&program_counter,sizeof(int));

    enviar_paquete(paquete, socket_memoria);

    char* instruccion_a_ejecutar = recibir_instruccion(socket_memoria);
    program_counter++;
    log_info(logger_cpu, "Program Counter actualizado: %d", program_counter);
    eliminar_paquete(paquete);
    return instruccion_a_ejecutar; // string instruccion y parametros pegados
}

instruccion* decode(char *inst){
    instruccion *instruccion_decodificada = malloc(sizeof(instruccion));//*))

    char **vector_terminos = string_split(inst, " ");

    u_int8_t identificador = get_identificador(vector_terminos[0]);

    for (int i = 0; i < string_array_size(vector_terminos); i++)
    {
        free(vector_terminos[i]);
    }
    

    free(vector_terminos);
    // if (identificador == SET){
    //     int dormir = (get_delay_cpu()) / 1000;
    //     sleep(dormir);
    // }
    instruccion_decodificada->identificador = identificador;
    instruccion_decodificada->cant_parametros = get_cant_parametros(identificador);
    instruccion_decodificada->parametros = get_parametros(inst, instruccion_decodificada->cant_parametros);
    free(inst);
    return instruccion_decodificada;
}

char* get_termino(char *instruccion, int index)
{
    char **vector_terminos = string_split(instruccion, " ");
    return vector_terminos[index];
}

t_list *get_parametros(char *inst, u_int8_t cant_parametros)
{   
    char **vector_terminos = string_split(inst, " ");
    t_list *parametros = list_create();
    for (int i = 1; i <= cant_parametros; i++)
    {   
        char* parametro=malloc(strlen(vector_terminos[i])+1); //+1
        strcpy(parametro, vector_terminos[i]);
        list_add(parametros, parametro);
    }
    for (int i = 0; i < string_array_size(vector_terminos); i++)
    {
        free(vector_terminos[i]);
    }
    
    free(vector_terminos); //SE LIBERAN LOS CHAR*?
    return parametros;
}


instruccion* execute(instruccion* inst) 
{
    switch (inst->identificador)
    {
    case SET:
        mostrar_parametros(inst, inst->cant_parametros);
        set(inst);
        break;
    case SUM:
        mostrar_parametros(inst, inst->cant_parametros);
        sum(inst);
        break;
    case SUB:
        mostrar_parametros(inst, inst->cant_parametros);
        sub(inst);
        break;
    case SLEEP:
        mostrar_parametros(inst, inst->cant_parametros);
        sleep_(inst);
        break;
    case JNZ:
        mostrar_parametros(inst, inst->cant_parametros);
        jnz(inst);
        break;
    case EXIT:
        mostrar_parametros(inst, inst->cant_parametros);
        exit_(inst);
        break;
    case WAIT: //  solicitar una instancia del recurso indicado por parametro.
        mostrar_parametros(inst, inst->cant_parametros);
        wait(inst);
        break;
    case SIGNAL: // liberar una instancia del recurso indicado por parametro.
        mostrar_parametros(inst, inst->cant_parametros);
        signal_(inst);
        break;
    case F_OPEN:
        mostrar_parametros(inst, inst->cant_parametros);
        f_open(inst);
        break;
    case F_CLOSE:
        mostrar_parametros(inst, inst->cant_parametros);
        f_close(inst);
        break;
    case MOV_IN:
        mostrar_parametros(inst, inst->cant_parametros);
        mov_in(inst);
        break;
    case MOV_OUT:
        mostrar_parametros(inst, inst->cant_parametros);
        mov_out(inst);
        break;
    case F_TRUNCATE:
        mostrar_parametros(inst, inst->cant_parametros);
        f_truncate(inst);
        break;
    case F_SEEK:
        mostrar_parametros(inst, inst->cant_parametros);
        f_seek(inst);
        break;
 case F_READ: 
        mostrar_parametros(inst, inst->cant_parametros);
        f_read(inst);
        break;
    case F_WRITE: 
        mostrar_parametros(inst, inst->cant_parametros);
        f_write(inst);
        break;
    case NO_RECONOCIDO:
        printf("se leyo mal el identificador\n");
        ejecutando_un_proceso=false;
        break;
    default:
        break;
    }
    return inst;
}

int traducir_direcciones(instruccion* inst,int direccion_logica){
    int dir = floor(direccion_logica / tam_pag);
    return dir;
}

//SET Registro Valor: Asigna al registro el valor pasado como parametro.
void set(instruccion *inst)
{
    uint32_t *dir_registro = get_direccion_registro(list_get(inst->parametros, 0));
    *dir_registro = atoi(list_get(inst->parametros, 1));
}

//SUM (Registro_Destino, Registro_Origen): Suma ambos registros y deja el resultado en el Destino.
void sum(instruccion *inst){
    char *registro_destino = list_get(inst->parametros, 0);
    char *registro_origen = list_get(inst->parametros, 1);
    
    uint32_t* registro_des =  get_direccion_registro(registro_destino); // direccion del registro destino
    uint32_t* registro_ori =  get_direccion_registro(registro_origen); // dreccion del registro origen
    
    uint32_t aux = *registro_des + *registro_ori;
    
    *registro_des = aux;
}

//SUB (Registro_Destino, Registro_Origen): Resta al Destino el Origen y lo deja en Destino.
void sub(instruccion *inst){
    char *registro_destino = list_get(inst->parametros, 0);
    char *registro_origen = list_get(inst->parametros, 1);
    
    uint32_t* registro_des =  get_direccion_registro(registro_destino); // direccion del registro destino
    uint32_t* registro_ori =  get_direccion_registro(registro_origen); // direccion del registro origen
    
    uint32_t aux = *registro_des - *registro_ori;
    
    *registro_des = aux;
}

//JNZ (Registro, Instrucción): Si el valor del registro es distinto de cero, 
//actualiza el program counter al número de instrucción pasada por parámetro.
void jnz(instruccion* inst){
    uint32_t *dir_registro = get_direccion_registro(list_get(inst->parametros, 0));
    char *pcf = list_get(inst->parametros, 1);

    if(*dir_registro != 0){
        int program_counter_futuro = atoi(pcf);
        program_counter = program_counter_futuro;
    }
}

//SLEEP (Tiempo): Esta instrucción representa una syscall bloqueante. 
//Se deberá devolver el Contexto de Ejecución actualizado al Kernel 
//junto a la cantidad de segundos que va a bloquearse el proceso.
void sleep_(instruccion* inst){
    devolver_contexto_de_ejecucion(SLEEP, inst);
    ejecutando_un_proceso = false;
}

//WAIT (Recurso): Esta instrucción solicita al Kernel que se asigne una instancia 
//del recurso indicado por parámetro.
void wait(instruccion *inst){
    devolver_contexto_de_ejecucion(WAIT, inst);
    ejecutando_un_proceso = false;
}

//SIGNAL (Recurso): Esta instrucción solicita al Kernel que se libere una instancia
// del recurso indicado por parámetro.
void signal_(instruccion* inst){
    devolver_contexto_de_ejecucion(SIGNAL, inst);
    ejecutando_un_proceso = false;
}

//MOV_IN (Registro, Dirección Lógica): Lee el valor de memoria correspondiente a la 
//Dirección Lógica y lo almacena en el Registro.
void mov_in(instruccion* inst){
    uint32_t *dir_registro = get_direccion_registro(list_get(inst->parametros, 0)); 
    char* dir_log = list_get(inst->parametros, 1);
    int direccion_logica = atoi(dir_log);

    int nro_pag = traducir_direcciones(inst, direccion_logica);

    t_paquete* pedir_marco = crear_paquete(PEDIDO_MARCO);
    agregar_a_paquete_solo(pedir_marco, &pid, sizeof(int));
    agregar_a_paquete_solo(pedir_marco, &nro_pag, sizeof(int));
    enviar_paquete(pedir_marco, socket_memoria);
    eliminar_paquete(pedir_marco);

    int cod_op = recibir_operacion(socket_memoria);
    int size=0;
    void* buffer = recibir_buffer(&size, socket_memoria);
    if(cod_op == ENVIAR_MARCO){
        int marco;
        memcpy(&marco, buffer, sizeof(int));
        int desplazamiento = direccion_logica - nro_pag * tam_pag;

        log_info(logger_cpu,"PID: <%d> - OBTENER MARCO - Página: <%d> - Marco: <%d>",pid,nro_pag,marco);

        t_paquete* solicitud_dato = crear_paquete(MOV_IN);
        agregar_a_paquete_solo(solicitud_dato, &marco, sizeof(int));
        agregar_a_paquete_solo(solicitud_dato, &desplazamiento, sizeof(int));
        enviar_paquete(solicitud_dato, socket_memoria);
        eliminar_paquete(solicitud_dato);

        cod_op = recibir_operacion(socket_memoria);
        int size_=0;
        void* buffer_dato = recibir_buffer(&size_, socket_memoria);
        uint32_t dato_a_guardar;
        memcpy(&dato_a_guardar, buffer_dato, sizeof(uint32_t));
        *dir_registro = dato_a_guardar;

        log_info(logger_cpu,"PID: <%d> - Acción: <LEER> - Dirección Física: <%d|%d> - Valor: <%d>",pid,marco,desplazamiento,dato_a_guardar);


        // cod_op = recibir_operacion(socket_memoria);
        // buffer = recibir_buffer(&size,socket_memoria);

        free(buffer_dato);
    }
    if(cod_op == PAGE_FAULT){
        program_counter--;
        inst->cant_parametros++;
        list_add(inst->parametros, string_itoa(nro_pag));

        log_info(logger_cpu,"Page Fault PID: <%d> - Página: <%d>",pid,nro_pag);
        
        devolver_contexto_de_ejecucion(PAGE_FAULT, inst);
        ejecutando_un_proceso = false;
    }
    free(buffer);
}

//MOV_OUT (Dirección Lógica, Registro): Lee el valor del Registro y lo escribe en la dirección 
//física de memoria obtenida a partir de la Dirección Lógica.
void mov_out(instruccion* inst){
    uint32_t* dir_registro = get_direccion_registro(list_get(inst->parametros, 1));
    char* dir_log = list_get(inst->parametros, 0);
    int direccion_logica = atoi(dir_log);

    int nro_pag = traducir_direcciones(inst, direccion_logica);

    t_paquete* pedir_marco = crear_paquete(PEDIDO_MARCO);
    agregar_a_paquete_solo(pedir_marco, &pid, sizeof(int));
    agregar_a_paquete_solo(pedir_marco, &nro_pag, sizeof(int));
    enviar_paquete(pedir_marco, socket_memoria);
    eliminar_paquete(pedir_marco);

    int cod_op = recibir_operacion(socket_memoria);
    int size=0;
    void* buffer = recibir_buffer(&size, socket_memoria);
    if(cod_op == ENVIAR_MARCO){
        int marco;
        memcpy(&marco, buffer, sizeof(int));
        int desplazamiento = direccion_logica - nro_pag * tam_pag;

        log_info(logger_cpu,"PID: <%d> - OBTENER MARCO - Página: <%d> - Marco: <%d>",pid,nro_pag,marco);

        t_paquete* envio_de_dato = crear_paquete(MOV_OUT);
        agregar_a_paquete_solo(envio_de_dato, &marco, sizeof(int));
        agregar_a_paquete_solo(envio_de_dato, &desplazamiento, sizeof(int));
        agregar_a_paquete_solo(envio_de_dato, dir_registro, sizeof(uint32_t));
        enviar_paquete(envio_de_dato, socket_memoria);
        eliminar_paquete(envio_de_dato);

        log_info(logger_cpu,"PID: <%d> - Acción: <ESCRIBIR> - Dirección Física: <%d|%d> - Valor: <%d>",pid,marco,desplazamiento,*dir_registro);
      
        int size_rta = 0;
        cod_op = recibir_operacion(socket_memoria);
        void* buffer_rta = recibir_buffer(&size_rta,socket_memoria);
        free(buffer_rta);
    }
    if(cod_op == PAGE_FAULT){
        bool pf;
        memcpy(&pf, buffer, sizeof(bool));
        program_counter--;
        list_add(inst->parametros, string_itoa(nro_pag));
        inst->cant_parametros++;

        log_info(logger_cpu,"Page Fault PID: <%d> - Página: <%d>",pid,nro_pag);

        devolver_contexto_de_ejecucion(PAGE_FAULT, inst);
        ejecutando_un_proceso = false;
    }
    free(buffer);

}

//F_OPEN (Nombre Archivo, Modo Apertura): Esta instrucción solicita al kernel que abra 
//el archivo pasado por parámetro con el modo de apertura indicado.
void f_open(instruccion* inst){
    devolver_contexto_de_ejecucion(F_OPEN, inst);
    ejecutando_un_proceso = false;
}

//F_CLOSE (Nombre Archivo): Esta instrucción solicita al kernel 
//que cierre el archivo pasado por parámetro.
void f_close(instruccion* inst){
    devolver_contexto_de_ejecucion(F_CLOSE, inst);
    ejecutando_un_proceso = false;
}

//F_SEEK (Nombre Archivo, Posición): Esta instrucción solicita al kernel actualizar 
//el puntero del archivo a la posición pasada por parámetro.
void f_seek(instruccion* inst){
    devolver_contexto_de_ejecucion(F_SEEK, inst);
    ejecutando_un_proceso = false;
}

//F_READ (Nombre Archivo, Dirección Lógica): Esta instrucción solicita al Kernel que se lea 
//del archivo indicado y se escriba en la dirección física de Memoria la información leída.
void f_read(instruccion *inst){
    char* dir_log = list_get(inst->parametros, 1);
    int direccion_logica = atoi(dir_log);

    int nro_pag = traducir_direcciones(inst, direccion_logica);
    t_paquete* pedir_marco = crear_paquete(PEDIDO_MARCO);
    agregar_a_paquete_solo(pedir_marco, &pid, sizeof(int));
    agregar_a_paquete_solo(pedir_marco, &nro_pag, sizeof(int));
    enviar_paquete(pedir_marco, socket_memoria);
    eliminar_paquete(pedir_marco);

    int cod_op = recibir_operacion(socket_memoria);
    int size;
    void* buffer = recibir_buffer(&size, socket_memoria);
    if(cod_op == ENVIAR_MARCO){
        int marco=0;
        memcpy(&marco,buffer,sizeof(int));
        inst->cant_parametros++;
        list_add(inst->parametros, string_itoa(marco));
        inst->cant_parametros++;
        list_add(inst->parametros, string_itoa(tam_pag));

        log_info(logger_cpu,"PID: <%d> - OBTENER MARCO - Página: <%d> - Marco: <%d>",pid,nro_pag,marco);

        devolver_contexto_de_ejecucion(F_READ, inst);
        ejecutando_un_proceso = false;
    }
    if(cod_op == PAGE_FAULT){
        program_counter--;
        inst->cant_parametros++;
        list_add(inst->parametros, string_itoa(nro_pag));

        log_info(logger_cpu,"Page Fault PID: <%d> - Página: <%d>",pid,nro_pag);

        devolver_contexto_de_ejecucion(PAGE_FAULT, inst);
        ejecutando_un_proceso = false;
    }
    free(buffer);
}

//F_WRITE (Nombre Archivo, Dirección Lógica): Esta instrucción solicita al Kernel que se 
//escriba en el archivo indicado la información que es obtenida a partir de la dirección física 
//de Memoria.
void f_write(instruccion *inst){
    char* dir_log = list_get(inst->parametros, 1);
    int direccion_logica = atoi(dir_log);

    int nro_pag = traducir_direcciones(inst, direccion_logica);
    t_paquete* pedir_marco = crear_paquete(PEDIDO_MARCO);
    agregar_a_paquete_solo(pedir_marco, &pid, sizeof(int));
    agregar_a_paquete_solo(pedir_marco, &nro_pag, sizeof(int));
    enviar_paquete(pedir_marco, socket_memoria);
    eliminar_paquete(pedir_marco);

    int cod_op = recibir_operacion(socket_memoria);
    int size;
    void* buffer = recibir_buffer(&size, socket_memoria);
    if(cod_op == ENVIAR_MARCO){
        int marco=0;
        memcpy(&marco,buffer,sizeof(int));
        inst->cant_parametros++;
        list_add(inst->parametros, string_itoa(marco));
        inst->cant_parametros++;
        list_add(inst->parametros, string_itoa(tam_pag));

        log_info(logger_cpu,"PID: <%d> - OBTENER MARCO - Página: <%d> - Marco: <%d>",pid,nro_pag,marco);

        devolver_contexto_de_ejecucion(F_WRITE, inst);
        ejecutando_un_proceso = false;
    }
    if(cod_op == PAGE_FAULT){
        program_counter--;
        inst->cant_parametros++;
        list_add(inst->parametros, string_itoa(nro_pag));

        log_info(logger_cpu,"Page Fault PID: <%d> - Página: <%d>",pid,nro_pag);
    
        devolver_contexto_de_ejecucion(PAGE_FAULT, inst);
        ejecutando_un_proceso = false;
    }
    free(buffer);
}

//F_TRUNCATE (Nombre Archivo, Tamaño): Esta instrucción solicita al Kernel que se modifique 
//el tamaño del archivo al indicado por parámetro.
void f_truncate(instruccion* inst){
    devolver_contexto_de_ejecucion(F_TRUNCATE, inst);
    ejecutando_un_proceso = false;
}

uint32_t* get_direccion_registro(char* string_registro){
    uint32_t* registro;
    
    if (!strcmp(string_registro, "AX"))
    {
        registro = &registros_cpu.AX;
    }
    if (!strcmp(string_registro, "BX"))
    {
        registro = &registros_cpu.BX;
    }
    if (!strcmp(string_registro, "CX"))
    {
        registro = &registros_cpu.CX;
    }
    if (!strcmp(string_registro, "DX"))
    {
        registro = &registros_cpu.DX;
    }

    return registro;
}

// adaptar
void mostrar_parametros(instruccion *inst, int cant_parametros)
{
    if (cant_parametros == 0)
    {
        log_info(logger_cpu, "PID: <%u> - Ejecutando: <%s> - <>", pid, get_motivo(inst->identificador));
    }
    if (cant_parametros == 1)
    {
        char *param1 = list_get(inst->parametros, 0);
        log_info(logger_cpu, "PID: <%u> - Ejecutando: <%s> - <%s>", pid, get_motivo(inst->identificador), param1);
    }
    if (cant_parametros == 2)
    {
        char *param1 = list_get(inst->parametros, 0);
        char *param2 = list_get(inst->parametros, 1);
        log_info(logger_cpu, "PID: <%u> - Ejecutando: <%s> - <%s,%s>\n", pid, get_motivo(inst->identificador),param1, param2);
    }
}

//EXIT: representa la syscall de finalización del proceso.
//Devuelve el Contexto de Ejecución actualizado al Kernel para su finalizacion.
void exit_(instruccion *inst){ 
    ejecutando_un_proceso = false;
    devolver_contexto_de_ejecucion(EXIT, inst);
}

//se usa en sleep y exit
void devolver_contexto_de_ejecucion(int motivo, instruccion *info){
    t_paquete *paquete_contexto = crear_paquete(CONTEXTO_EJECUCION);
    empaquetar_contexto(paquete_contexto, motivo, info);
    enviar_paquete(paquete_contexto, socket_kernel_dispatch);
    eliminar_paquete(paquete_contexto);
}

void empaquetar_contexto(t_paquete *paquete_contexto, int motivo, instruccion *info){
	agregar_a_paquete_solo(paquete_contexto, (&program_counter), sizeof(int));
	empaquetar_registros_cpu(paquete_contexto, registros_cpu);
	agregar_a_paquete_solo(paquete_contexto, &motivo, sizeof(int)); // SE LE SUMA EL MOTIVO
	
    agregar_a_paquete_solo(paquete_contexto, &(info->cant_parametros), sizeof(int));

	for (int i = 0; i < info->cant_parametros; i++)
	{
		char *parametro = list_get(info->parametros, i);
		agregar_a_paquete(paquete_contexto, parametro, strlen(parametro) + 1);
	}
}

char* recibir_instruccion(int socket_cliente){
	int size=0;
    recibir_operacion(socket_cliente);
 	char* buffer = recibir_buffer(&size, socket_cliente);
    
	return buffer;
}

u_int8_t get_identificador(char *identificador_leido)
{
	u_int8_t identificador = NO_RECONOCIDO;
	if (!strcmp("F_WRITE", identificador_leido))
		identificador = F_WRITE;
	if (!strcmp("F_READ", identificador_leido))
		identificador = F_READ;
	if (!strcmp("SET", identificador_leido))
		identificador = SET;
	if (!strcmp("MOV_IN", identificador_leido))
		identificador = MOV_IN;
	if (!strcmp("MOV_OUT", identificador_leido))
		identificador = MOV_OUT;
	if (!strcmp("F_TRUNCATE", identificador_leido))
		identificador = F_TRUNCATE;
	if (!strcmp("F_SEEK", identificador_leido))
		identificador = F_SEEK;
	if (!strcmp("WAIT", identificador_leido))
		identificador = WAIT;
	if (!strcmp("SIGNAL", identificador_leido))
		identificador = SIGNAL;
	if (!strcmp("F_OPEN", identificador_leido))
		identificador = F_OPEN;
	if (!strcmp("F_CLOSE", identificador_leido))
		identificador = F_CLOSE;
	if (!strcmp("EXIT", identificador_leido))
		identificador = EXIT;
	if (!strcmp("SUM", identificador_leido))
		identificador = SUM;
    if (!strcmp("SUB", identificador_leido))
	    identificador = SUB;
    if (!strcmp("SLEEP", identificador_leido))
		identificador = SLEEP;
    if (!strcmp("JNZ", identificador_leido))
	    identificador = JNZ;
	return identificador;
}
