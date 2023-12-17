#include "Memoria.h"

sem_t mlog;
t_list* lista_paginas_cargadas; //contiene pidPags
t_list* lista_referenciadas;    //contiene pidPags
void *memoria;
t_bitarray* bitmap_frames;

/* -------------------------------------Iniciar Memoria -----------------------------------------------*/
int main(int argc, char **argv){
	config_memoria = iniciar_config(argv[1]);
	logger_memoria=iniciar_logger("memoria.log","MEMORIA");
    memoria = calloc(1, tamanio_memoria());
    inicializar_bitmap_frames();
    char* algoritmo = algoritmo_reemplazo();

    if(strcmp(algoritmo, "FIFO")==0){
        lista_paginas_cargadas = list_create();
    }
    else{
        if(strcmp(algoritmo, "LRU")==0)
        lista_referenciadas = list_create();
    }
    procesos_memoria = list_create();
    sem_init(&mlog, 1, 1);
    int socket_servidor, socket_cliente, socket_kernel, socket_cpu, socket_fs;

    log_protegido_mem(string_from_format("Iniciando Conexiones..."));
    char *puerto = config_get_string_value(config_memoria, "PUERTO_ESCUCHA");
    log_protegido_mem(string_from_format("Iniciando servidor..."));
    socket_servidor = iniciar_servidor(puerto);
    
    if (socket_servidor == -1)
    {
        log_error(logger_memoria, "ERROR - No se pudo crear el servidor");
        return EXIT_FAILURE;
    }
    
    log_protegido_mem(string_from_format("Servidor listo para recibir clientes"));

    pthread_t hilo_kernel, hilo_cpu, hilo_atender_fs;

    // socket_cliente = esperar_cliente(socket_servidor);
    // pthread_create(&hilo_fs_servidor,NULL,(void*)conectarFS,NULL);
 
    for (int i = 0; i < 3; i++)
    {
        log_protegido_mem(string_from_format("Esperando Cliente..."));
        socket_cliente = esperar_cliente(socket_servidor);

        int cod_op = recibir_operacion(socket_cliente);

        switch (cod_op)
        {
        case KERNEL:
            log_protegido_mem(string_from_format("Se conecto el KERNEL"));
            socket_kernel = socket_cliente;
            pthread_create(&hilo_kernel, NULL, (void *)conectarKernel, &socket_kernel);
            break;
        case CPU:
            log_protegido_mem(string_from_format("Se conecto la CPU"));
            socket_cpu = socket_cliente;
            pthread_create(&hilo_cpu, NULL, (void *)conectarCpu, &socket_cpu);
            break;
        case FS:
            log_protegido_mem(string_from_format("Se conecto el FILE SYSTEM"));
            socket_fs = socket_cliente;
            pthread_create(&hilo_atender_fs, NULL, (void *)atenderFS, &socket_fs);
            break; 
        default:
            log_protegido_mem(string_from_format("No reconozco ese codigo"));
            break;
        }
    }

    pthread_join(hilo_kernel, NULL);
    free(memoria);
    free(bitmap_frames->bitarray);
    bitarray_destroy(bitmap_frames);
    if(strcmp(algoritmo, "FIFO")==0){
        list_destroy(lista_paginas_cargadas);
    }
    else{
        if(strcmp(algoritmo, "LRU")==0)
        list_destroy(lista_referenciadas);
    }

    for (int i = 0; i < list_size(procesos_memoria); i++)
    {   
        t_proceso* proceso=list_get(procesos_memoria,i);
        for (int j = 0; j < list_size(proceso->instrucciones); j++)
        {
            free(list_get(proceso->instrucciones,j));
        }
        list_destroy(proceso->instrucciones);

        for (int j = 0; j < list_size(proceso->tdp); j++)
        {
            free(list_get(proceso->tdp,j));
        }
        list_destroy(proceso->tdp);
        free(proceso);
    }
    list_destroy(procesos_memoria);


    pthread_join(hilo_cpu, NULL);
    config_destroy(config_memoria);
    liberar_conexion(socket_peticion_FS);
    
    pthread_join(hilo_atender_fs,NULL); //CON JOIN DA MAS ML
    log_destroy(logger_memoria); // NO LLEGA NUNCA VER-----------------------------------------------

    return EXIT_SUCCESS;
}

/* ------------------------------------Conexion CPU --------------------------------------------------*/
int conectarCpu(int* socket_cpu){

    log_protegido_mem(string_from_format("Enviando tamanio de pagina..."));
    int tam_pagina = tamanio_pagina();
    send(*socket_cpu, &tam_pagina, sizeof(int), 0);
    log_protegido_mem(string_from_format("Tamanio de pagina enviado"));

    while (true)
    {
        void* buffer;
        // log_protegido_mem(string_from_format( "Esperando peticiones de CPU..."));

        int cod_cpu = recibir_operacion(*socket_cpu);
        int size=0;
        int pid=0;
        switch (cod_cpu){
        case MENSAJE:
            recibir_mensaje(*socket_cpu,logger_memoria);
            break;
        case FETCH_INSTRUCCION:
            buffer = recibir_buffer((&size), *socket_cpu);
            
            int program_counter;
            printf("BUSCANDO INSTRUCCION....\n");
            memcpy(&pid, buffer , sizeof(int));
            memcpy(&program_counter, buffer  + sizeof(int), sizeof(int));
            
            usleep(retardo_memoria()*1000);//retardo de la obtencion de la instruccion

            char* instruccion=buscar_instruccion_proceso(pid, program_counter);
            
            enviar_mensaje(instruccion, *socket_cpu);
            // t_paquete* paquete_instruccion= crear_paquete(FETCH_INSTRUCCION);

			// agregar_a_paquete(paquete_instruccion, instruccion, strlen(instruccion)+1);
            // enviar_paquete(paquete_instruccion, *socket_cpu);
            break;
        case PEDIDO_MARCO:                      //BUSCAR MARCO DE PAGINA
            int pagina=0;
            int desplazamiento = 0;

            buffer  = recibir_buffer(&size, *socket_cpu);      // cpu manda pid y pagina
            memcpy(&pid, buffer , sizeof(int));
            desplazamiento += sizeof(int);
            memcpy(&pagina, buffer  + desplazamiento, sizeof(int));
            enviar_marco_asociado(pid, pagina, socket_cpu);                 //ACA LE MANDO A CPU EL MARCO ASOCIADO A LA DIREC LOGICA
            break;
        
//mov in (direccion fisica)       ----------->>>>>    Nos dan direc y les devolvemos el dato
        case MOV_IN:
            int marco_in=0;
            int desplazamiento_movin=0;
            uint32_t bytes_a_copiar=0;
            int desp_buffer = 0;

            buffer= recibir_buffer(&size, *socket_cpu);
            memcpy(&marco_in, buffer , sizeof(int));
            desp_buffer += sizeof(int);
            memcpy(&desplazamiento_movin, buffer  + desp_buffer, sizeof(int));

            int pos_inicial = marco_in * tamanio_pagina() + desplazamiento_movin;
            
            t_pidPag* pidPag_asociado_in = buscar_proceso_asociado_a(marco_in); 
            
            log_protegido_mem(string_from_format("PID: <%d> - Accion: <LEER> - Direccion Fisica: <%d|%d>",pidPag_asociado_in->pid, marco_in, desplazamiento_movin));

            void* puntero = memoria;
            puntero += pos_inicial;

            memcpy(&bytes_a_copiar,puntero , sizeof(uint32_t));

            t_paquete* paquete_dato = crear_paquete(DATO_REGISTRO);
            agregar_a_paquete(paquete_dato, &bytes_a_copiar, sizeof(uint32_t));
            enviar_paquete(paquete_dato, *socket_cpu);
            eliminar_paquete(paquete_dato);
            free(pidPag_asociado_in);
            break;

//MOV_OUT (direccion fisica, dato)  ----------->>>>>    Nos dan dato del registro y lo escribimos en memoria
        case MOV_OUT:
            int marco=0;
            int despl_out=0;
            uint32_t dato_a_copiar=0;
            
            buffer= recibir_buffer(&(size), *socket_cpu);
            memcpy(&marco, buffer , sizeof(int));
            memcpy(&despl_out, buffer  + sizeof(int), sizeof(int));
            memcpy(&dato_a_copiar, buffer  + sizeof(int)*2, sizeof(uint32_t));
            printf("COPIAMOS EL DATO %d EN MEMORIA--------------\n",dato_a_copiar);
            
            int pos_inicial_out = marco * tamanio_pagina() + despl_out;
            printf("La posicion inicial es: %d\n", pos_inicial_out);

            t_pidPag* pidPag_asociado_out = buscar_proceso_asociado_a(marco);
            t_proceso* proc_modificado = buscar_proceso(procesos_memoria ,pidPag_asociado_out->pid);
            tabla_paginas* pagina_en_tdp = list_get(proc_modificado->tdp, pidPag_asociado_out->pagina);
            pagina_en_tdp->modificado = 1;
            
            log_protegido_mem(string_from_format("PID: <%d> - Accion: <ESCRIBIR> - Direccion fisica: <%d|%d>", pidPag_asociado_out->pid, marco, despl_out));

            char* posicion_a_escribir = memoria;
            posicion_a_escribir += pos_inicial_out;

            memcpy(posicion_a_escribir, &dato_a_copiar, sizeof(uint32_t));

            // void* posicion_a_escribir = (char*)memoria + pos_inicial_out;
            // memcpy(posicion_a_escribir, &dato_a_copiar, sizeof(uint32_t));
            
            enviar_mensaje("OK", *socket_cpu);

            free(pidPag_asociado_out);
            break;
        case -1:
            sem_wait(&mlog);
            log_error(logger_memoria, "La CPU se ha desconectado.");
            sem_post(&mlog);
            return EXIT_FAILURE;
        default:
            sem_wait(&mlog);
            log_error(logger_memoria, "Operacion desconocida.");
            sem_post(&mlog);
            break;
        }
        free(buffer);
    }
    
    return 0;
}

/* ------------------------------------Conexion Kernel ----------------------------------------------*/
int conectarKernel(int* socket_kernel){
    log_protegido_mem(string_from_format("Enviando mensaje de confirmacion..."));

    bool confirmacion = true;
    send(*socket_kernel, &confirmacion, sizeof(bool), 0);
    log_protegido_mem(string_from_format("Mensaje enviado"));
    int size;
    void *buffer;

    while (1){
        //int pid;
        log_protegido_mem(string_from_format("Esperando peticiones de KERNEL..."));
        int cod_kernel = recibir_operacion(*socket_kernel);

        switch (cod_kernel){
        case MENSAJE:
            recibir_mensaje(*socket_kernel,logger_memoria);
            break;
        case INICIAR:
            //int size=0;
            buffer = recibir_buffer(&size, *socket_kernel);
            int pid=0;
            double size_proceso;
            int size_path=0;
            char* path;
            int desplazamiento = 0;

            memcpy(&pid, buffer +desplazamiento, sizeof(int));
            desplazamiento+=sizeof(int);
            memcpy(&size_proceso, buffer +desplazamiento, sizeof(double));
            desplazamiento+=sizeof(double);
            memcpy(&size_path,buffer +desplazamiento, sizeof(int));
            desplazamiento+=sizeof(int);
            path=malloc(size_path);
            memcpy(path, buffer +desplazamiento, size_path);

            printf("PATH: %s\n",path);
            
            FILE *f;
	        if (!(f = fopen(path, "r"))) {
                log_error(logger_memoria, "No se encontro el archivo de instrucciones");
                return -1;
	        }
            free(path);
            t_proceso* proceso_nuevo = agregar_proceso_instrucciones(f,pid);
            
            crear_tabla_de_paginas(size_proceso,proceso_nuevo);
            pedir_bloques_swap(proceso_nuevo);
           
            bool confirmacion = recibir_pos_swap(proceso_nuevo);
            send(*socket_kernel, &confirmacion, sizeof(bool), 0); //devuelve 1 (confirmado) o 0(error)
            break;
        case LIBERAR_ESPACIO_PROCESO:
            int pid_a_liberar=0;

            buffer  = recibir_buffer(&size, *socket_kernel);
            memcpy(&pid_a_liberar, buffer , sizeof(int));

            log_protegido_mem(string_from_format("Eliminacion de Proceso PID: <%d>", pid_a_liberar));

            liberar_memoria_espacio_usuario(pid_a_liberar);
        break;
        case PAGE_FAULT:  // kernel avisa que hay que resolverlo            
	    int p_id=0;
            int pag=0;
    
            buffer  = recibir_buffer(&size, *socket_kernel);
            memcpy(&p_id, buffer, sizeof(int));
            memcpy(&pag, buffer + sizeof(int), sizeof(int));
            resolver_page_fault(p_id,pag);
            enviar_mensaje("OK",*socket_kernel);
            break;
        case -1:
            log_error(logger_memoria,"El KERNEL se desconecto");
            return EXIT_FAILURE;
        default:
            log_warning(logger_memoria, "Operacion desconocida.");
            break;
        }
        free(buffer);
    }
    return 0;
}


/* ------------------------------------Atender FS --------------------------------------------------*/
int atenderFS(int* socket_fs){
    log_protegido_mem(string_from_format("Enviando mensaje de confirmacion..."));
    bool confirmacion = true;

    send(*socket_fs, &confirmacion, sizeof(bool), 0);

    pthread_t hilo_fs_servidor;
    pthread_create(&hilo_fs_servidor,NULL,(void*)conectarFS,NULL);
    pthread_detach(hilo_fs_servidor);

    log_protegido_mem(string_from_format("Mensaje enviado."));

    void *buffer=NULL;
    int size=0;
    while (true){
        log_protegido_mem(string_from_format("Esperando peticiones del FILE SYSTEM..."));

        int cod_op = recibir_operacion(*socket_fs);
        buffer= recibir_buffer(&(size), *socket_fs);

        usleep(retardo_memoria()*1000);
        switch (cod_op){

        case ESCRITURA_MEMORIA:                   //Leer archivo. escribimos lo recibido por FS y le enviamos OK de confirmacion
            int marco_escr=0;
            int pid_escr=0;
            void *bufferDePaginaACargar;
            int bytesAEscribir=0;
            int despl_buffer = 0;
            char* alg_reemplazo = algoritmo_reemplazo();

            memcpy(&marco_escr, buffer, sizeof(int));
            despl_buffer += sizeof(int);            
            
            int pos_inicial = marco_escr * tamanio_pagina();
            
            memcpy(&bytesAEscribir, buffer + despl_buffer, sizeof(int));
            despl_buffer += sizeof(int);

            bufferDePaginaACargar=malloc(bytesAEscribir);

            memcpy(bufferDePaginaACargar, buffer + despl_buffer, bytesAEscribir);
            
            despl_buffer += bytesAEscribir;

            memcpy(&pid_escr, buffer + despl_buffer, sizeof(int));

            memcpy(memoria + pos_inicial, bufferDePaginaACargar, bytesAEscribir);          // CARGAMOS LA PAG EN LA MEMORIA
            
            if(strcmp(alg_reemplazo,"LRU")==0){
                t_pidPag* pidPag_a_actualizar = buscar_proceso_asociado_a(marco_escr);
                agregar_a_referenciadas(pid_escr, pidPag_a_actualizar->pagina);               //ACTUALIZAR LISTA REFERENCIAS
                free(pidPag_a_actualizar);
            }
            
            log_protegido_mem(string_from_format("PID: <%d> - Accion: <ESCRIBIR> - Direccion fisica: <%d>", pid_escr, pos_inicial));

            free(bufferDePaginaACargar);
            enviar_mensaje("OK", *socket_fs);
            break;

        case LECTURA_MEMORIA:                       //Escribir archivo. leemos dato de una DF y se lo pasamos a FS
            int marco_lec=0;
            int pid_lec=0;
            int bytesALeer=0;
            void *bufferPaginaALeer;
            int despl=0;
            char* alg_reemplazo_lectura = algoritmo_reemplazo();

            memcpy(&marco_lec, buffer ,sizeof(int));
            despl+=sizeof(int);
            printf("COPIAMOS EL DATO QUE ESTA EN EL MARCO %d------------------------\n",marco_lec);

            memcpy(&bytesALeer, buffer +despl, sizeof(int));
            despl+=sizeof(int);
            printf("BYTES A COPIAR DE MEMORIA %d----------------------------\n",bytesALeer);
            memcpy(&pid_lec, buffer +despl,sizeof(int));

            bufferPaginaALeer = malloc(bytesALeer);

            int pos_inic = marco_lec * tamanio_pagina();

            void* posicion_a_leer = memoria + pos_inic;
            memcpy(bufferPaginaALeer, posicion_a_leer, bytesALeer);

            printf("SE COPIA DE MEMORIA %s  PARA QUE SE ESCRIBA EN FS-------------\n",(char*)bufferPaginaALeer);
            printf("SE COPIA DE MEMORIA %s  PARA QUE SE ESCRIBA EN FS-------------\n",(char*)(bufferPaginaALeer+ sizeof(int)));
            printf("SE COPIA DE MEMORIA %s  PARA QUE SE ESCRIBA EN FS-------------\n",(char*)(bufferPaginaALeer+ 2*sizeof(int)));
            printf("SE COPIA DE MEMORIA %s  PARA QUE SE ESCRIBA EN FS-------------\n",(char*)(bufferPaginaALeer+ 3*sizeof(int)));

            t_paquete* paquete_pagina = crear_paquete(PAQUETE);
            agregar_a_paquete_solo(paquete_pagina, bufferPaginaALeer , bytesALeer);
            enviar_paquete(paquete_pagina,*socket_fs);
            
            if(strcmp(alg_reemplazo_lectura,"LRU")==0){
                t_pidPag* pidPag_actualizar = buscar_proceso_asociado_a(marco_lec);
                agregar_a_referenciadas(pid_lec, pidPag_actualizar->pagina);                //ACTUALIZAR LISTA REFERENCIAS
                free(pidPag_actualizar);
            }

            log_protegido_mem(string_from_format("PID: <%d> - Accion: <ESCRIBIR> - Direccion fisica: <%d>", pid_lec, pos_inic));

            eliminar_paquete(paquete_pagina);
            free(bufferPaginaALeer);
            break;
        case -1:
            log_error(logger_memoria, "El FS se ha desconectado.");
            free(buffer);
            return EXIT_FAILURE;
            
        default:
            log_error(logger_memoria, "Operacion desconocida.");
            break;

        }
        free(buffer);
    }
    return 0;
}
/* ------------------------------------Conexion Servidor FS --------------------------------------------------*/

int conectarFS(){
	char* ip;
	char* puerto;

	ip= config_get_string_value(config_memoria,"IP_FILESYSTEM");
	puerto=config_get_string_value(config_memoria,"PUERTO_FILESYSTEM");

    // log_protegido_mem(string_from_format("Lei la IP %s y  puerto %s\n",ip,puerto));

    socket_peticion_FS = crear_conexion(ip, puerto);

    if (socket_peticion_FS<0){
        printf("No se pudo establecer una conexion con el FileSystem \n");
    }
    else{
		log_protegido_mem(string_from_format("Conexion con FileSystem exitosa"));
    }

    int handshake = MEMORIA;
	bool confirmacion;
	send(socket_peticion_FS, &handshake, sizeof(int),0);
	recv(socket_peticion_FS, &confirmacion, sizeof(bool), MSG_WAITALL);

	if(confirmacion)
		log_protegido_mem(string_from_format("Handshake con FILE SYSTEM exitoso"));
	else
		log_protegido_mem(string_from_format("ERROR: Handshake con FILE SYSTEM  fallido"));

return 0;
}

void crear_tabla_de_paginas(double size_proceso, t_proceso* proceso_nuevo){
    int tam_pag = tamanio_pagina();
    int cant_paginas = ceil((size_proceso/tam_pag));//+0.5

    proceso_nuevo->tdp = list_create();

    for (int i = 0; i < cant_paginas; i++)
    {
        tabla_paginas* pagina=malloc(sizeof(tabla_paginas));
        pagina->marco = 0;
        pagina->presencia = 0;
        pagina->modificado = 0;
        pagina->posicion_swap = 0;
        list_add(proceso_nuevo->tdp,pagina);
    }
   
    //(tabla_paginas*)malloc(cant_paginas*sizeof(tabla_paginas)); //tdp como  (segun avril)    
    proceso_nuevo->cant_paginas=cant_paginas;
    log_protegido_mem(string_from_format("Creacion de Tabla de Paginas PID: <%d> - Tamanio: <%d>", proceso_nuevo->id, proceso_nuevo->cant_paginas));
}

void pedir_bloques_swap(t_proceso* proceso_nuevo){
    t_paquete* proceso_swap = crear_paquete(ASIGNAR_BLOQUE_SWAP);
    agregar_a_paquete_solo(proceso_swap, &(proceso_nuevo->cant_paginas), sizeof(int));
    enviar_paquete(proceso_swap, socket_peticion_FS);
    eliminar_paquete(proceso_swap);
}

bool recibir_pos_swap(t_proceso* proceso_nuevo){
//agregar_a_paquete(paquete_memoria, bloquesAsignados, cantidadBloques*sizeof(int));
    int size=0;
    int desplazamiento = 0;

    int cod_op = recibir_operacion(socket_peticion_FS);

    void* buffer;
    
    if(cod_op == BLOQUES_ASIGNADOS){                                         //DECIRLE A CAMI QUE LO MANDE ASÍ
        buffer  = recibir_buffer(&size, socket_peticion_FS);
        memcpy(&size, buffer , sizeof(int));
        desplazamiento += sizeof(int);

        for(int i=0; i<proceso_nuevo->cant_paginas; i++){
            tabla_paginas* renglon_tdp = list_get(proceso_nuevo->tdp, i);

            memcpy(&(renglon_tdp->posicion_swap), buffer + desplazamiento, sizeof(int));
            desplazamiento += sizeof(int);
        }
        free(buffer);
            return true;
    }else return false;
}

void liberar_memoria_espacio_usuario(int pid){
    t_proceso *proceso_a_eliminar = buscar_proceso(procesos_memoria, pid);
    t_paquete* posicionesA_liberar = crear_paquete(LIBERAR_BLOQUES);
    agregar_a_paquete_solo(posicionesA_liberar, &proceso_a_eliminar->cant_paginas, sizeof(int));
    for(int i=0; i<proceso_a_eliminar->cant_paginas; i++){
        tabla_paginas* pagina_en_tdp = list_get(proceso_a_eliminar->tdp,i);
        agregar_a_paquete_solo(posicionesA_liberar, &pagina_en_tdp->posicion_swap, sizeof(int));
        if(pagina_en_tdp->presencia){
            sacar_pagina_memoria(pid,i);            //liberar cada pagina del proceso del void* memoria
        }
    }

    enviar_paquete(posicionesA_liberar, socket_peticion_FS);
    eliminar_paquete(posicionesA_liberar);

    for (int i = 0; i < list_size(proceso_a_eliminar->tdp); i++)
    {
        free(list_get(proceso_a_eliminar->tdp,i));
    }
    // for (int i = 0; i >=0 list_size(proceso_a_eliminar->tdp); i++)
    // {
    //     free(list_get(proceso_a_eliminar->tdp,i));
    // }


    list_destroy(proceso_a_eliminar->tdp);  //elimino renglon a renglon la tabla de paginas

    for (int i = 0; i < list_size(proceso_a_eliminar->instrucciones); i++)
    {
        free(list_get(proceso_a_eliminar->instrucciones,i));
    }
    
    list_destroy(proceso_a_eliminar->instrucciones);


    log_protegido_mem(string_from_format("Destruccion de Tabla de Paginas PID: <%d> - Tamanio: <%d>", proceso_a_eliminar->id, proceso_a_eliminar->cant_paginas));

    list_remove_element(procesos_memoria,proceso_a_eliminar);      //elimino proceso de lista de procesos
    free(proceso_a_eliminar);
}

int buscar_marco( int pid, int pagina){                               //si la pag tiene presencia = 1 devuelve el marco, sino devuelve -1
    t_proceso *proceso = buscar_proceso(procesos_memoria, pid);

    tabla_paginas* pagina_en_tdp = list_get(proceso->tdp, pagina);

    if(pagina_en_tdp->presencia){           
        return pagina_en_tdp->marco;
    }else return -1;
}

void enviar_marco_asociado(int pid, int pagina, int *socket_cpu){
    int marco_buscado = buscar_marco(pid, pagina); 
    
    if(marco_buscado != -1){        //-1 es page fault
        t_paquete* paquete_marco = crear_paquete(ENVIAR_MARCO);
        agregar_a_paquete_solo(paquete_marco, &marco_buscado, sizeof(int));
        enviar_paquete(paquete_marco, *socket_cpu);
        eliminar_paquete(paquete_marco);
	char* alg_reemplazo = algoritmo_reemplazo();
        if(strcmp(alg_reemplazo,"LRU")==0){
            agregar_a_referenciadas(pid, pagina);
        }
    }else{      //CASO PF
        bool page_fault = true;
        t_paquete* paquete_pf = crear_paquete(PAGE_FAULT);
        agregar_a_paquete_solo(paquete_pf, &page_fault, sizeof(bool));          //Le mando que hay PF en un bool
        enviar_paquete(paquete_pf, *socket_cpu);
        eliminar_paquete(paquete_pf);
    }
}

/*void* buffer=crear_buffer();*/

int resolver_page_fault(int pid, int pagina){ //carga pagina faltante y devuelve el frame en el que se cargó
    int marco=0;
    if(chequeo_mem_llena()){
        char* alg_reemplazo = algoritmo_reemplazo();
        marco = seleccionar_victima(alg_reemplazo); //devuelve frame a usar

        t_pidPag* proceso_victima = buscar_proceso_asociado_a(marco);

        int pid_victima = proceso_victima->pid;
        int pagina_victima = proceso_victima->pagina;

        log_protegido_mem(string_from_format("REEMPLAZO - Marco: <%d> - Page Out: <%d>-<%d> - Page In: <%d>-<%d>",marco, pid_victima, pagina_victima, pid, pagina));

        sacar_pagina_memoria(pid_victima,pagina_victima);
        
        cargar_pagina(pid, pagina, marco);
        
        free(proceso_victima);
    }
    else{
        marco = buscar_frame_libre();
        if(marco == -1){
            log_error(logger_memoria,"ERROR de implementacion. La memoria debería tener frames libres\n");
            return marco;
        }
        cargar_pagina(pid, pagina, marco);  
    }
    return marco;
}

int buscar_frame_libre(){                       //tiene que devolver SI O SI el nro de frame libre
    for(int i=0; i<frames_totales();i++){       //no puede no haber frame libre porq ya chequeamos que la memoria no esté llena
        if(bitmap_frames->bitarray[i]==0) //if(&bitmap_frames[i]==0)
            return i;
    }
    return -1;
}

void agregar_a_lista_paginas(int pid, int pag){
    t_pidPag* pagina_a_agregar=malloc(sizeof(t_pidPag));

    pagina_a_agregar->pid=pid;
    pagina_a_agregar->pagina=pag;
    
    list_add(lista_paginas_cargadas, pagina_a_agregar);   //esta bien cargado pagina_a_agregar?
}

int seleccionar_victima(char *algoritmo){   //SELECCIONA Y LIBERA
    t_proceso *proc_victima;

    if(strcmp(algoritmo, "FIFO")== 0){
        //lista_paginas_cargadas = [(pid=5, pagina=2),(pid=3, pagina=4),(pid=1, pagina=2)]
        t_pidPag* victima = list_get(lista_paginas_cargadas, 0);
        proc_victima = buscar_proceso(procesos_memoria ,victima->pid);
        tabla_paginas* pagina_en_tdp = list_get(proc_victima->tdp, victima->pagina);
        
        // sacar_pagina_memoria(victima->pid,victima->pagina);
        
        return pagina_en_tdp->marco;
        
    }else if(strcmp(algoritmo, "LRU")== 0){
        //lista_referenciadas = [(pid=3, pagina=4),(pid=1, pagina=2),(pid=5, pagina=2)]  
        t_pidPag* victima = list_get(lista_referenciadas, 0);
        proc_victima = buscar_proceso(procesos_memoria ,victima->pid);
        tabla_paginas* pagina_en_tdp = list_get(proc_victima->tdp, victima->pagina);
        // sacar_pagina_memoria(victima->pid,victima->pagina);
        return pagina_en_tdp->marco;
    } 
    //VER ESTO 
    return -1;
}

void sacar_pagina_memoria(int pid, int pag){
    t_proceso* proceso_padre = buscar_proceso(procesos_memoria ,pid);
    tabla_paginas* pagina_en_tdp = list_get(proceso_padre->tdp, pag);
    char* algoritmo = algoritmo_reemplazo();

    if(pagina_en_tdp->modificado){              //si la pagina está modificada hay que escribirla en swap
        t_paquete* paqueteFS = crear_paquete(ESCRIBIR_SWAP);
        int pos_swap = pagina_en_tdp->posicion_swap;
        
        agregar_a_paquete_solo(paqueteFS, &pos_swap, sizeof(int));
        agregar_a_paquete_solo(paqueteFS, &(pagina_en_tdp->marco), sizeof(int));
        agregar_a_paquete_solo(paqueteFS, &pid, sizeof(int));

        enviar_paquete(paqueteFS, socket_peticion_FS);           //NO SE QUE SOCKET ES
        eliminar_paquete(paqueteFS);

        log_protegido_mem(string_from_format("SWAP OUT -  PID: <%d> - Marco: <%d> - Page Out: <%d>-<%d>", pid, pagina_en_tdp->marco , pid, pag));

        leer_pagina(pid, pag); //cuando le digo a fs que quiero escribir un bloque, me pide leer la pag correspondiente a ese bloque
    }

    // Posición inicial desde donde se sacan bytes (inicio de frame)
        int pos_inicial = pagina_en_tdp->marco * tamanio_pagina();          //byte
        int tamanio_a_liberar = tamanio_pagina();    // Cantidad de bytes que se van a eliminar (tamaño de pagina)
        void* ptr = (void*)memoria + pos_inicial;
        memset(ptr, 0, tamanio_a_liberar);           // los seteo en 0
 
    //actualizar estructuras asociadas
        if(strcmp(algoritmo, "FIFO")==0 && !list_is_empty(lista_paginas_cargadas)){
            for (int i = list_size(lista_paginas_cargadas)-1 ; i >= 0 ; i--)
            {
                t_pidPag* pidPag=list_get(lista_paginas_cargadas,i);
                printf("Marco %d pid %d pag %d\n",i,pidPag->pid,pidPag->pagina);
                if(pidPag->pid==pid && pidPag->pagina==pag){
                    free(list_remove(lista_paginas_cargadas,i));   //sacar elemento de lista_paginas y lberar espacio
                }
            }
        }
        else{
            if(strcmp(algoritmo, "LRU")==0 && !list_is_empty(lista_referenciadas)){
                for (int i =list_size(lista_referenciadas)-1; i >= 0 ; i--)
                {
                    t_pidPag* pidPag=list_get(lista_referenciadas,i);
                    printf("Marco %d pid %d pag %d\n",i,pidPag->pid,pidPag->pagina);
                    if(pidPag->pid==pid && pidPag->pagina==pag){
                        free(list_remove(lista_referenciadas,i));   //sacar elemento de lista_paginas y lberar espacio
                    }
                }
            }
        }
        pagina_en_tdp->presencia=0;
        pagina_en_tdp->modificado=0;

        bitmap_frames->bitarray[pagina_en_tdp->marco] = 0;

}

void leer_pagina(int pid, int pag){
    int cod_op = recibir_operacion(socket_peticion_FS); // cod_op seria LECTURA_MEMORIA
    void* buffer;
    void* bufferPaginaALeer = malloc(tamanio_pagina());
    int size;
    int marco;
    int pos_inicial;
    char* algoritmo = algoritmo_reemplazo();              

    if(cod_op == LECTURA_MEMORIA){
        buffer  = recibir_buffer(&size, socket_peticion_FS);
        memcpy(&marco, buffer ,sizeof(int));

        pos_inicial = marco * tamanio_pagina();
        memcpy(bufferPaginaALeer, memoria+pos_inicial, tamanio_pagina());

        t_paquete* paquete_pagina = crear_paquete(PAQUETE);
        agregar_a_paquete_solo(paquete_pagina, bufferPaginaALeer , tamanio_pagina());
        enviar_paquete(paquete_pagina,socket_peticion_FS);
        eliminar_paquete(paquete_pagina);

        if(strcmp(algoritmo, "LRU")==0){
            agregar_a_referenciadas(pid, pag);
        }

    }else{
        printf("Se esperaba cod_op LECTURA_MEMORIA. Se recibió mal. \n");
    }

    free(bufferPaginaALeer);
    free(buffer);
}

void agregar_a_referenciadas(int pid, int pag){
    int indice = buscar_pidPag(lista_referenciadas,pid,pag);
    if(indice==-1){ // si el pidpag no esta en la lista
        t_pidPag* pidpag = malloc(sizeof(t_pidPag));
        pidpag->pid = pid;
        pidpag->pagina = pag;
        list_add(lista_referenciadas, pidpag);
    }else{
        t_pidPag* aux = list_remove(lista_referenciadas, indice);
        list_add(lista_referenciadas, aux);
    }
}

int buscar_pidPag(t_list* lista, int pid, int pag){  //-1 si no lo encuentra
    t_pidPag* aux; //= malloc(sizeof(t_pidPag));
    for(int i=0; i<list_size(lista);i++){
        aux=list_get(lista,i);
        if(aux->pid==pid && aux->pagina==pag){
            return i;
        }
    }
    //free(aux);
    return -1;
}

bool chequeo_mem_llena(){
    bool mem_llena = false;
    if(strcmp("FIFO", algoritmo_reemplazo())==0){
        mem_llena = list_size(lista_paginas_cargadas) == frames_totales();
    }
    else if (strcmp("LRU", algoritmo_reemplazo())==0){
        mem_llena = list_size(lista_referenciadas) == frames_totales();
    }
    return mem_llena;
}

void cargar_pagina(int pid, int pagina, int marco){
    char* algoritmo = algoritmo_reemplazo();
    t_proceso* proceso = buscar_proceso(procesos_memoria,pid);
    tabla_paginas* pagina_en_tdp = list_get(proceso->tdp,pagina);

    pedir_pagina_FS(pid, pagina, marco);  //LEER_SWAP
    
    log_protegido_mem(string_from_format("SWAP IN -  PID: <%d> - Marco: <%d> - Page In: <%d>-<%d>", pid, marco, pid, pagina));

    recibir_pagina_FS();                  //ESCRIBIR_MEMORIA
    
    
    if(strcmp(algoritmo, "FIFO")==0){
        if(list_size(lista_paginas_cargadas)<(tamanio_memoria()/tamanio_pagina())){
            agregar_a_lista_paginas(pid, pagina);
        }
        else log_error(logger_memoria,"Memoria llena");
    }
    else if(strcmp(algoritmo, "LRU")==0){
        if(list_size(lista_referenciadas)<(tamanio_memoria()/tamanio_pagina())){
            agregar_a_referenciadas(pid, pagina);
        }
        else log_error(logger_memoria,"Memoria llena");
    }

    pagina_en_tdp->marco = marco;            //actualizacion tdp
    pagina_en_tdp->presencia = 1;

    bitmap_frames->bitarray[marco]=1;
    // bitarray_set_bit(bitmap_frames,marco);   //actualizacion bitmap    
    
}

void pedir_pagina_FS(int pid, int pagina, int marco){
    //pedir pagina a swap
    t_proceso* proceso = buscar_proceso(procesos_memoria, pid);
    tabla_paginas* pagina_en_tdp = list_get(proceso->tdp, pagina);

    int numero_bloque = pagina_en_tdp->posicion_swap; 

    t_paquete* pedir_bloque = crear_paquete(LEER_SWAP);

    agregar_a_paquete_solo(pedir_bloque, &numero_bloque, sizeof(int));
    agregar_a_paquete_solo(pedir_bloque, &marco, sizeof(int));               //le mando el marco nomas, porq desde FS trata al desplazamiento como si fuera un 0
    agregar_a_paquete_solo(pedir_bloque, &pid, sizeof(int));
    
    enviar_paquete(pedir_bloque, socket_peticion_FS); 
    eliminar_paquete(pedir_bloque);
}

void recibir_pagina_FS(){
    //RECIBIR PAGINA  
    void *buffer;
    void *bufferDePaginaACargar = malloc(tamanio_pagina());
    int size = 0;               
    int cod_op = recibir_operacion(socket_peticion_FS);
    int frame = 0;
    int pid = 0;
    int tamanio = 0;

    if(cod_op != ESCRITURA_MEMORIA){
        printf("Se esperaba cod_op = ESCRITURA_MEMORIA. Se recibio mal.\n");
        return;
    }

    buffer  = recibir_buffer(&size, socket_peticion_FS);

    memcpy(&frame, buffer , sizeof(int));
    int pos_inicial = frame * tamanio_pagina();                  // donde debo cargarla

    memcpy(&tamanio, buffer + sizeof(int), sizeof(int));
    memcpy(bufferDePaginaACargar, buffer  + sizeof(int)*2, tamanio);    // sería &bufferDePaginaACargar?
    memcpy(&pid, buffer  + 2*sizeof(int) + tamanio, sizeof(int));

    memcpy(memoria + pos_inicial, bufferDePaginaACargar, tamanio_pagina());          // CARGAMOS LA PAG EN LA MEMORIA

    free(bufferDePaginaACargar);
    free(buffer);
    enviar_mensaje("Escritura ok", socket_peticion_FS);
}

void inicializar_bitmap_frames(){
    int n_frames = frames_totales();
    char *bufferBitmap = malloc(n_frames); 
    bitmap_frames = bitarray_create_with_mode(bufferBitmap, n_frames, LSB_FIRST);

    // limpiar el bitArray (recorre seteando a 0)
    for (int i = 0; i < n_frames; i++)
    {
        bitmap_frames->bitarray[i]=0;
        // bitarray_clean_bit(bitmap_frames, i);
    }
    //bitarray_set_bit()
    //test_bit (0 es libre)
}

t_pidPag* buscar_proceso_asociado_a(int frame){  //DESPUES DE USARLO SIEMPRE HACER FREE
    t_pidPag* pidPag = malloc(sizeof(t_pidPag));

    for (int i = 0; i < list_size(procesos_memoria); i++){
        t_proceso* proceso_asociado = list_get(procesos_memoria, i);
        
        for (int j=0; j < proceso_asociado->cant_paginas; j++){
            tabla_paginas* pagina_en_tdp = list_get(proceso_asociado->tdp, j);
            
            if(pagina_en_tdp->marco == frame && pagina_en_tdp->presencia == 1){
                pidPag->pid = proceso_asociado->id;
                pidPag->pagina = j;
            return pidPag;
            }
        }
    }
    return NULL;
}

void log_protegido_mem(char *mensaje){
    sem_wait(&mlog);
    log_info(logger_memoria, "%s", mensaje);
    sem_post(&mlog);
    free(mensaje);
}

int tamanio_memoria(){
    return atoi(config_get_string_value(config_memoria,"TAM_MEMORIA"));
}

int tamanio_pagina(){
    return atoi(config_get_string_value(config_memoria,"TAM_PAGINA"));
}

int retardo_memoria(){
    return atoi(config_get_string_value(config_memoria,"RETARDO_RESPUESTA"));
}

int frames_totales(){ //int?
    int tam_mem = tamanio_memoria();
    int tam_pag = tamanio_pagina();
    return (tam_mem/tam_pag);
}

char* algoritmo_reemplazo(){
    return config_get_string_value(config_memoria,"ALGORITMO_REEMPLAZO");
}
