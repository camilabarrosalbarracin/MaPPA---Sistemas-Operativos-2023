#include "FS.h"

int socket_memoria_servidor;
int socket_memoria, socket_kernel;
sem_t mlog;

t_log *logger_FS()
{
    t_log *loggerFS;
    loggerFS = log_create("FS.log", "FILE SYSTEM", 1, LOG_LEVEL_DEBUG);

    if (loggerFS == NULL)
    {
        printf("No pude crear el log FS\n");
        exit(3);
    }
    return loggerFS;
}

/* -------------------------------------Iniciar FS ---------------------------------------------------*/
int main(int argc, char **argv) {
	config_fs = iniciar_config(argv[1]);
    logger_fs = logger_FS();
    sem_init(&mlog,1,1);
    char* path_fcb = path_FCB();

    inicializar_FAT();
    inicializar_bloques();
    inicializar_bitmap_swap();
    mkdir(path_fcb, 0777); // crea la carpeta de los fcb (si no existe)

    
	pthread_t hilo_memoria_servidor, hilo_memoria, hilo_kernel;

    int socket_servidor, socket_cliente;

    log_protegido_fs(string_from_format("Iniciando Conexiones..."));
    char *puerto = config_get_string_value(config_fs, "PUERTO_ESCUCHA");
    log_protegido_fs(string_from_format("Iniciando servidor..."));
    socket_servidor = iniciar_servidor(puerto);
    
    if (socket_servidor == -1)
    {
        log_error(logger_fs, "ERROR - No se pudo crear el servidor");
        return EXIT_FAILURE;
    }
    
    log_protegido_fs(string_from_format("Servidor listo para recibir clientes"));

    pthread_create(&hilo_memoria_servidor,NULL,(void*)conectarMemoria,NULL);
 
    for (int i = 0; i < 2; i++)
    {
        log_protegido_fs(string_from_format("Esperando Cliente..."));
        socket_cliente = esperar_cliente(socket_servidor);

        int cod_op = recibir_operacion(socket_cliente);

        switch (cod_op)
        {
        case KERNEL:
            log_protegido_fs(string_from_format("Se conecto el KERNEL"));
            socket_kernel = socket_cliente;
            pthread_create(&hilo_kernel, NULL, (void *)conectarKernel, NULL);
            // printf("%d\n",socket_kernel);
            break;
        case MEMORIA:
            log_protegido_fs(string_from_format("Se conecto la MEMORIA"));
            socket_memoria = socket_cliente;
            pthread_create(&hilo_memoria, NULL, (void *)atenderMemoria, NULL);
            break;
        default:
            log_protegido_fs(string_from_format("No reconozco ese codigo"));
            break;
        }
    }

    pthread_join(hilo_kernel, NULL);

    liberar_conexion(socket_memoria_servidor);

    pthread_join(hilo_memoria, NULL);
    pthread_join(hilo_memoria_servidor, NULL);

    log_destroy(logger_fs);
    config_destroy(config_fs);
    free(bitmap_swap->bitarray);
    bitarray_destroy(bitmap_swap);
    return EXIT_SUCCESS;
}

/* ------------------------------------Conexion Kernel -----------------------------------------------*/
int conectarKernel(){
    log_protegido_fs(string_from_format("Enviando mensaje de confirmacion..."));
    bool confirmacion = true;

    send(socket_kernel, &confirmacion, sizeof(bool), 0);
    log_protegido_fs(string_from_format("Mensaje enviado."));

    //t_buffer *buffer = crear_buffer();

    while (true){
        // log_protegido_fs(string_from_format("Esperando peticiones del KERNEL..."));

        int cod_op = recibir_operacion(socket_kernel);
        int tam=0;
        int pid;
        int bytes;
        int puntero;
        char* nombre;
        int cantcar=0;
        int dirfisica;
        void* buffer = recibir_buffer(&tam,socket_kernel);
        int desplazamiento = 0;
        char* ok = "OK";
        switch (cod_op){

        case MENSAJE:
            // recibir_mensaje(socket_kernel, logger_fs);
            log_info(logger_fs, "Me llego el mensaje %p\n", buffer);
            break;
        case F_OPEN:
            memcpy(&cantcar,buffer,sizeof(int));
            nombre=malloc(cantcar);
            memcpy(nombre,buffer + sizeof(int),cantcar);
            abrir_archivo(nombre);
            free(nombre);
            break;
        // case F_CREATE:
        //     memcpy(&cantcar,buffer,sizeof(int));
        //     nombre = malloc(cantcar);
        //     memcpy(nombre,buffer + sizeof(int),cantcar);
        //     crear_archivo(nombre);
        //     enviar_mensaje(ok,socket_kernel);
        //     free(nombre);
        //     break;
        case F_TRUNCATE:
            memcpy(&cantcar,buffer,sizeof(int));
            nombre = malloc(cantcar);
            memcpy(nombre,buffer + sizeof(int),cantcar);
            memcpy(&bytes,buffer+sizeof(int)+cantcar,sizeof(int));
            truncar_archivo(nombre,bytes);

            enviar_mensaje(ok,socket_kernel);
            log_info(logger_fs, "Truncar Archivo: %s - Tamanio: %d", nombre, bytes);

            free(nombre);
            break;
        case F_READ:
            memcpy(&cantcar,buffer,sizeof(int));
            desplazamiento+=sizeof(int);
            nombre = malloc(cantcar);
            memcpy(nombre,buffer + desplazamiento,cantcar);
            desplazamiento+=cantcar;
            memcpy(&puntero,buffer+desplazamiento,sizeof(int));
            desplazamiento+=sizeof(int);
            
            memcpy(&bytes,buffer+desplazamiento,sizeof(int));
            desplazamiento+=sizeof(int);
            memcpy(&dirfisica,buffer+desplazamiento,sizeof(int));
            desplazamiento+=sizeof(int);
            memcpy(&pid,buffer+desplazamiento,sizeof(int));
            desplazamiento+=sizeof(int);
            leer_archivo(nombre,puntero,bytes,dirfisica,pid);
            
            enviar_mensaje(ok,socket_kernel);
            free(nombre);
            break;
        case F_WRITE:
            memcpy(&cantcar,buffer,sizeof(int));
            desplazamiento+=sizeof(int);
            nombre = malloc(cantcar);
            memcpy(nombre,buffer + desplazamiento,cantcar);
            desplazamiento+=cantcar;
            memcpy(&puntero,buffer+desplazamiento,sizeof(int));
            desplazamiento+=sizeof(int);
            memcpy(&bytes,buffer+desplazamiento,sizeof(int));
            desplazamiento+=sizeof(int);
            
            memcpy(&dirfisica,buffer+desplazamiento,sizeof(int));
            desplazamiento+=sizeof(int);
            
            memcpy(&pid,buffer+desplazamiento,sizeof(int));
            desplazamiento+=sizeof(int);

            escribir_archivo(nombre,puntero,bytes,dirfisica,pid);

            enviar_mensaje(ok,socket_kernel);

            free(nombre);
            break;
        case -1:
            log_error(logger_fs, "El Kernel se desconecto. Terminando servidor");
            free(buffer);
            return EXIT_FAILURE;
        default:
            log_warning(logger_fs, "Operacion desconocida.");
            break;
        }
        free(buffer);
    }

    return EXIT_SUCCESS;
}



/* ------------------------------------Conexion Mermoria --------------------------------------------*/
int conectarMemoria(){
    socket_memoria_servidor = -1;
    char *ip;
    char *puerto;

    ip = config_get_string_value(config_fs, "IP_MEMORIA");
    puerto = config_get_string_value(config_fs, "PUERTO_MEMORIA");

    socket_memoria_servidor = crear_conexion(ip, puerto);

    if (socket_memoria_servidor <= 0){
        printf("No se pudo establecer una conexion con la memoria \n");
    }

    int identificador = FS;
    bool confirmacion;
    send(socket_memoria_servidor, &identificador, sizeof(int), 0);

    recv(socket_memoria_servidor, &confirmacion, sizeof(bool), MSG_WAITALL);
    if (confirmacion){
        log_info(logger_fs, "Conexion con memoria exitosa");
    }

    //config_destroy(config_fs);
    return 0;
}


/* ------------------------------------Atender Mermoria --------------------------------------------*/

int atenderMemoria(){
    log_protegido_fs(string_from_format("Enviando mensaje de confirmacion..."));
    bool confirmacion = true;
    int bloque_swap=0;
    int direccion_Fisica;
    int tamanioBloque = tamanio_bloque();
    int pid=0;

    send(socket_memoria, &confirmacion, sizeof(bool), 0);
    log_protegido_fs(string_from_format("Mensaje enviado."));

    void *buffer;
    int size=0;

    while (true){

        int cod_op = recibir_operacion(socket_memoria);
        buffer = recibir_buffer(&size, socket_memoria); // CHEQUEAR ESTO TAMBIEN

        switch (cod_op){

        case MENSAJE:
            recibir_mensaje(socket_memoria, logger_fs);

            break;
        case PAQUETE:
            log_info(logger_fs, "Me llegaron los siguientes valores:\n");
            
            break;
        case ASIGNAR_BLOQUE_SWAP:
            int bloques;
            memcpy(&bloques,buffer,sizeof(int));
            iniciar_proceso(bloques,socket_memoria); // Puede ser que haya que pasarle a la conexion al server de memoria (?)
            break;
        case LIBERAR_BLOQUES:
            int cant_bloques;
            memcpy(&cant_bloques,buffer,sizeof(int));
            void *bloquesALiberar=malloc(cant_bloques * sizeof(int));
            memcpy(bloquesALiberar,(buffer) + sizeof(int),cant_bloques * sizeof(int));
            finalizar_proceso(bloquesALiberar,cant_bloques);
            free(bloquesALiberar);
            break;
        case LEER_SWAP:
            memcpy(&bloque_swap,buffer,sizeof(int));
            memcpy(&direccion_Fisica,buffer + sizeof(int),sizeof(int));
            memcpy(&pid,buffer + sizeof(int)*2,sizeof(int));
            leer_archivo("", bloque_swap, tamanioBloque, direccion_Fisica, pid);
            break;
        case ESCRIBIR_SWAP:
            int marco=0;
            memcpy(&bloque_swap,buffer,sizeof(int));
            memcpy(&marco,buffer + sizeof(int),sizeof(int));
            memcpy(&pid,buffer + sizeof(int)*2,sizeof(int));

            printf("pos swap %d, marco %d, pid %d\n",bloque_swap,marco,pid);
            escribir_archivo("", bloque_swap, tamanioBloque, marco, pid);
            break;
        case -1:
            log_error(logger_fs, "La MEMORIA se desconecto. Terminando servidor");
            free(buffer);
            return EXIT_FAILURE;
        default:
            log_warning(logger_fs, "Operacion desconocida.");
            break;
        }
        free(buffer);
    }
    return EXIT_SUCCESS;
}



void log_protegido_fs(char *mensaje){
    sem_wait(&mlog);
    log_info(logger_fs, "%s", mensaje);
    free(mensaje);
    sem_post(&mlog);
}


/* ------------------------------------ File System (inits) --------------------------------------------*/
void inicializar_FAT(){
    char *path_FAT_ = path_FAT(); // Salva el path del config
    int FAT = open(path_FAT_, O_RDWR | O_CREAT, 0644); // abre archivo (write-read, sino crea)
    struct stat s;// te da las caracteristicas del archivo
    fstat(FAT, &s);
    size_t tamArchivo = s.st_size;
    int cantidadBloques =  cant_bloques_total();
    int cantidadBloquesSWAP = cant_bloques_SWAP(); 

    int tamanioFAT = (cantidadBloques - cantidadBloquesSWAP) * sizeof(uint32_t);

    if (tamArchivo == 0){   // Asigno tamanio al archivo si lo creo (sino ya tiene tam)
        ftruncate(FAT, tamanioFAT); // cambia el tam del archivo
        uint32_t bufferAux[tamanioFAT];
        for (int i = 0; i < tamanioFAT; i++){
           bufferAux[i] = 0; // (setea a 0 las entradas)
        }
        write(FAT,bufferAux,tamanioFAT);
        bufferFAT = mmap(NULL, tamanioFAT, PROT_WRITE | PROT_READ, MAP_SHARED, FAT, 0);
        
        close(FAT);
        return;
    }

    bufferFAT = mmap(NULL, tamanioFAT, PROT_WRITE | PROT_READ, MAP_SHARED, FAT, 0);

    close(FAT);

    return;
}

void inicializar_bloques(){
    char *path_bloque = path_bloques();
    int bloques = open(path_bloque, O_RDWR | O_CREAT, 0644); // abre archivo (write-read, sino crea)
    struct stat s;                                           // te da las caracteristicas del archivo
    fstat(bloques, &s);
    size_t tamArchivo = s.st_size;
    int tamanioBloque = tamanio_bloque();
    int cantidadBloques = cant_bloques_total();


    int tamanioArchivo = tamanioBloque * cantidadBloques; // tamanio = tamBloque * cantBloq

    if (tamArchivo == 0){ // Asigno tamanio al archivo si lo creo (sino ya tiene tam)
        ftruncate(bloques, tamanioArchivo);
    }

    bufferBloques = mmap(NULL, tamanioArchivo, PROT_WRITE | PROT_READ, MAP_SHARED, bloques, 0); // trae de memoria las direcciones del archivo, tamaño,etc
    // mmap(NULL, tamanioArchivo, permisos ,MAP_SHARED,archivo , puntero(posicion))

    close(bloques);
}

void inicializar_bitmap_swap(){
    int tamanio_swap = cant_bloques_SWAP();
    char *bufferBitmap = malloc(tamanio_swap); //       es int pero deberia ser char* REVISAR 
    bitmap_swap = bitarray_create_with_mode(bufferBitmap, tamanio_swap, LSB_FIRST);

    // limpiar el bitArray (recorre seteando a 0)
    for (int i = 0; i < tamanio_swap; i++)
    {
        bitarray_clean_bit(bitmap_swap, i);
    }
}


/* ------------------------------------ Peticiones Modulo Kernel --------------------------------------------*/

//---------------------------------------------Abrir Archivo---------------------------------------------------
//La operación de abrir archivo consistirá en verificar que exista el FCB correspondiente al archivo. 
//En caso de que exista deberá devolver el tamaño del archivo.
//En caso de que no exista, deberá informar que el archivo no existe


void abrir_archivo(char *archivo){

    char *path_FCB_ = path_FCB();

    log_info(logger_fs, "Abrir Archivo <%s>", archivo);

    char *pathArchivo = (char *)malloc(strlen(path_FCB_) + strlen(archivo) + 3);
    strcpy(pathArchivo, path_FCB_);
    strcat(pathArchivo, "/");
    strcat(pathArchivo, archivo);

    FILE *fcb = NULL;
    fcb = fopen(pathArchivo, "r");


    if (fcb != NULL){
        t_config *fcb_archivo = iniciar_config(pathArchivo);
        int tamArchivo = config_get_int_value(fcb_archivo, "TAMANIO_ARCHIVO");
        t_paquete* paquete = crear_paquete(FILE_EXISTS);
        agregar_a_paquete(paquete,&tamArchivo,sizeof(int));
        enviar_paquete(paquete,socket_kernel);
        eliminar_paquete(paquete);
        config_destroy(fcb_archivo);
        fclose(fcb);
        free(pathArchivo);
        return;
    }
    else{
        int tamArchivo = 0;
        crear_archivo(archivo);
        t_paquete* paquete = crear_paquete(FILE_EXISTS);
        agregar_a_paquete(paquete,&tamArchivo,sizeof(int));
        enviar_paquete(paquete,socket_kernel);
        eliminar_paquete(paquete);
    }
    free(pathArchivo);
    return;
}

//---------------------------------------------Crear Archivo---------------------------------------------------
//En la operación crear archivo, se deberá crear un archivo FCB con tamaño 0 y sin bloque inicial.
//Siempre será posible crear un archivo y por lo tanto esta operación deberá devolver OK.

void crear_archivo(char *archivo){

    char *path_FCB_ = path_FCB();

    log_info(logger_fs, "Crear Archivo: %s", archivo);

    // ruta del archivo
    char *pathArchivo = string_from_format("%s/%s", path_FCB_, archivo);

    t_config *fcb_archivo = malloc(sizeof(t_config));
    fcb_archivo->path = string_duplicate(pathArchivo); // asigno el path
    fcb_archivo->properties = dictionary_create();
    // agrego propiedades del archivo
    char *cero = "0";
    dictionary_put(fcb_archivo->properties, "NOMBRE_ARCHIVO", string_duplicate(archivo));
    dictionary_put(fcb_archivo->properties, "TAMANIO_ARCHIVO", string_duplicate(cero));
    dictionary_put(fcb_archivo->properties, "BLOQUE_INICIAL", string_duplicate(cero));

    config_save(fcb_archivo);
    config_destroy(fcb_archivo);
    free(pathArchivo);
}

//---------------------------------------------Truncar Archivo---------------------------------------------------
//Al momento de truncar un archivo, pueden ocurrir 2 situaciones: 
//Ampliar el tamaño del archivo: Al momento de ampliar el tamaño del archivo deberá actualizar 
//el tamaño del archivo en el FCB y se le deberán asignar tantos bloques como sea necesario para
// poder direccionar el nuevo tamaño.
//Reducir el tamaño del archivo: Se deberá asignar el nuevo tamaño del archivo en el FCB y 
//se deberán marcar como libres todos los bloques que ya no sean necesarios para direccionar 
//el tamaño del archivo (descartando desde el final del archivo hacia el principio).
//Siempre se van a poder truncar archivos para ampliarlos, no se realizará la prueba de llenar el FS.

void truncar_archivo(char *archivo, int tamanioTruncar){
    
    char* path_fcb = path_FCB();
    int  tamanioBloque = tamanio_bloque();
    char* path_Archivo = string_from_format("%s/%s",path_fcb,archivo);
    char *string; //revisar
    
    t_config *fcb_archivo = iniciar_config(path_Archivo);// Abro el fcb del archivo
    free(path_Archivo);

    // Leo 
    int tamanio_Archivo = config_get_int_value(fcb_archivo,"TAMANIO_ARCHIVO");
    int bloque_inicial = config_get_int_value(fcb_archivo,"BLOQUE_INICIAL");

    if (tamanio_Archivo > tamanioTruncar){

        int bloquesASacar = ceil((tamanio_Archivo - tamanioTruncar) / (tamanioBloque));

        sacarBloques(bloquesASacar,bloque_inicial,archivo,tamanio_Archivo);

        string = string_from_format("%d", tamanioTruncar);
        config_set_value(fcb_archivo, "TAMANIO_ARCHIVO", string);
        free(string);
    }
    else{
        
        int bloquesAAgregar = ceil((tamanioTruncar - tamanio_Archivo) / (tamanioBloque));

        agregarBloques(bloquesAAgregar,&bloque_inicial,archivo);

        string = string_from_format("%d", tamanioTruncar);
        config_set_value(fcb_archivo, "TAMANIO_ARCHIVO", string);
        free(string);
        string = string_from_format("%d", bloque_inicial);
        config_set_value(fcb_archivo, "BLOQUE_INICIAL", string);
        free(string);

    }
    config_save(fcb_archivo);
    config_destroy(fcb_archivo);
    return;
}

void sacarBloques(int bloquesASacar, int bloque_inicial, char *nombre_archivo,int tamanio_Archivo){
    
    int tamanio_bloques = tamanio_bloque();
    int cantidad_bloques = ceil(tamanio_Archivo/tamanio_bloques);
    // Indices a bloques del archivo (salvo los bloques enlazados)
    int tamanio = cantidad_bloques*sizeof(int);
    void *punteros_archivo = malloc(tamanio);
    int desplazamiento;
    get_array_punteros(punteros_archivo,bloque_inicial,tamanio_Archivo); 
    int j = (cantidad_bloques-1); // Puntero para volver
    int endOfFile = UINT32_MAX;
    int bloque_libre = 0;
    int bloques_Aux = bloquesASacar;
    int retardo_fat = retardo_acceso_FAT(); // int a[3] a[1] a[2] *(a+1)
    int retardo_bloque = retardo_acceso_bloques();

    // Libero los bloques referidos en el array
    usleep(retardo_fat*1000);
    while(bloques_Aux != 0){
        if (bloques_Aux == 1){
            memcpy(&desplazamiento, punteros_archivo+(j*sizeof(int)), sizeof(int));
            memcpy(bufferFAT +desplazamiento* sizeof(int),&bloque_libre,sizeof(int));
            log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",desplazamiento,0);
            usleep(retardo_bloque*1000);
            usleep(retardo_fat*1000);
            j--;
            memcpy(&desplazamiento, punteros_archivo+(j*sizeof(int)), sizeof(int));
            memcpy(bufferFAT +desplazamiento* sizeof(int),&endOfFile,sizeof(int));
            log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d", desplazamiento,endOfFile);
            usleep(retardo_bloque*1000);
            usleep(retardo_fat*1000);
            break;
        } 
        memcpy(&desplazamiento, punteros_archivo+(j*sizeof(int)), sizeof(int));
        memcpy(bufferFAT +desplazamiento* sizeof(int),&bloque_libre,sizeof(int));
        log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",desplazamiento,0);
        usleep(retardo_bloque*1000);
        usleep(retardo_fat*1000);
        j--;
        bloques_Aux--;
    }
    int tam_FAT = cant_bloques_total();
    msync(bufferFAT,tam_FAT * sizeof(int),MS_INVALIDATE);
    free(punteros_archivo);
    return;
}

void agregarBloques(int bloquesAAgregar, int *puntero_bloque_inicial, char *nombre_archivo){

    int endOfFile = UINT32_MAX;
    int puntero_aux = *puntero_bloque_inicial; // Desp cambia al ultimo bloque
    int puntero_bloque_libre = 1; // primer bloque libre (tiene posiciones)
    int bloques_Aux = bloquesAAgregar; 
    int proximo_bloque;
    int retardo_fat = retardo_acceso_FAT();
    // Busco el puntero al ultimo bloque del archivo
    if(puntero_aux != 0){
        memcpy(&proximo_bloque,bufferFAT + (puntero_aux*sizeof(int)),sizeof(int));
        log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",puntero_aux,proximo_bloque);
        usleep(retardo_fat*1000);
        while(proximo_bloque != endOfFile){
            memcpy(&puntero_aux,bufferFAT + (puntero_aux*sizeof(int)),sizeof(int));
            memcpy(&proximo_bloque,bufferFAT + (puntero_aux*sizeof(int)),sizeof(int));
            log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",puntero_aux,proximo_bloque);
            usleep(retardo_fat*1000);
        }
    }
    

    // Asigno los bloques
    memcpy(&proximo_bloque,bufferFAT + (puntero_bloque_libre*sizeof(int)),sizeof(int));
    log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",puntero_bloque_libre,proximo_bloque);
    usleep(retardo_fat*1000);
    while(bloques_Aux != 0){
        // Busco el primer bloque libre
        while(proximo_bloque != 0){
            puntero_bloque_libre ++;
            memcpy(&proximo_bloque,bufferFAT + (puntero_bloque_libre*sizeof(int)),sizeof(int));
            log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",puntero_bloque_libre,proximo_bloque);
            usleep(retardo_fat*1000);
        }
        if (*puntero_bloque_inicial == 0){
            *puntero_bloque_inicial = puntero_bloque_libre;
            memcpy(bufferFAT + (*puntero_bloque_inicial*sizeof(int)),&endOfFile,sizeof(int));
            log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",*puntero_bloque_inicial,proximo_bloque);
            usleep(retardo_fat*1000);
            proximo_bloque = endOfFile;
            bloques_Aux--;
            puntero_aux = puntero_bloque_libre;
            continue;
        }
        memcpy(bufferFAT + (puntero_aux*sizeof(int)),&puntero_bloque_libre,sizeof(int));
        log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",puntero_aux,puntero_bloque_libre);
        usleep(retardo_fat*1000);
        memcpy(bufferFAT + (puntero_bloque_libre*sizeof(int)),&endOfFile,sizeof(int));
        log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",puntero_bloque_libre,endOfFile);
        usleep(retardo_fat*1000);
        proximo_bloque = endOfFile;
        puntero_aux = puntero_bloque_libre;
        bloques_Aux--;
    }
    int tam_FAT = cant_bloques_total();
    msync(bufferFAT,tam_FAT * sizeof(int),MS_INVALIDATE);
    return;
}

//---------------------------------------------Leer Archivo---------------------------------------------------
//Esta operación leerá la información correspondiente al bloque a partir del puntero, 
//siempre se leerá el contenido completo del bloque. 
//La información se deberá enviar al módulo Memoria para ser escrita a partir de 
//la dirección física recibida por parámetro, una vez recibida la confirmación por parte del módulo Memoria, 
//se informará al módulo Kernel del éxito de la operación.

void leer_archivo(char *archivo, int puntero, int bytesALeer, int direccionFisica, int pid){

    void *bufferDeDatosParaMemoria = malloc(bytesALeer);
    char *string;
    char *path_fcb = path_FCB();
    int tamanio_bloques = tamanio_bloque();
    int cant_bloques_swap = 0;
    int bloque_actual;
    int retardo = retardo_acceso_bloques();


    if(!string_is_empty(archivo)){
        log_info(logger_fs, "Leer Archivo: %s - Puntero: %d  - Memoria: %d - Tamaño: %d", archivo, puntero, direccionFisica, bytesALeer);
        // abro el fcb a partir del nombre de archivo del kernel.
        string = string_from_format("%s/%s", path_fcb, archivo);
        t_config *fcb_archivo = iniciar_config(string);
        free(string);

        // leo del fcb
        int bloque_inicial = config_get_int_value(fcb_archivo, "BLOQUE_INICIAL");
        int tamanio_archivo = config_get_int_value(fcb_archivo,"TAMANIO_ARCHIVO");
        int cantidad_bloques = ceil(tamanio_archivo/tamanio_bloques);
        int tamanio = cantidad_bloques*sizeof(int);
        void *punteros_archivo = malloc(tamanio); 
        get_array_punteros(punteros_archivo,bloque_inicial,cantidad_bloques);
        cant_bloques_swap = cant_bloques_SWAP();

        int primer_bloque = ceil(puntero/tamanio_bloques);


        memcpy(&bloque_actual,punteros_archivo+((primer_bloque)*sizeof(int)),sizeof(int));
        bloque_actual += cant_bloques_swap;
        memcpy(bufferDeDatosParaMemoria,bufferBloques + (bloque_actual*tamanio_bloques),tamanio_bloques);
        log_info(logger_fs, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d", archivo, primer_bloque, bloque_actual);
        usleep(retardo * 1000);

        free(punteros_archivo);
        config_destroy(fcb_archivo);
    }else{
        puntero+=1;
        memcpy(bufferDeDatosParaMemoria, bufferBloques+puntero*tamanio_bloques, tamanio_bloques);
        log_info(logger_fs, "Acceso Swap: %d", puntero);
        usleep(retardo*1000);
        
    }
    // MANDAR A MEMORIA
    if(!string_is_empty(archivo)){
    t_paquete *paqueteMemoria = crear_paquete(ESCRITURA_MEMORIA);
    agregar_a_paquete_solo(paqueteMemoria, &direccionFisica, sizeof(int));
    agregar_a_paquete(paqueteMemoria, bufferDeDatosParaMemoria, bytesALeer);
    agregar_a_paquete_solo(paqueteMemoria, &pid, sizeof(int));

    enviar_paquete(paqueteMemoria, socket_memoria_servidor);
    eliminar_paquete(paqueteMemoria);
    int tamanio_rta;
    recibir_operacion(socket_memoria_servidor);
    void *respuestaMemoria = recibir_buffer(&tamanio_rta, socket_memoria_servidor);
    free(respuestaMemoria);
    free(bufferDeDatosParaMemoria);
    }
    else{
    t_paquete *paqueteMemoria = crear_paquete(ESCRITURA_MEMORIA);
    agregar_a_paquete_solo(paqueteMemoria, &direccionFisica, sizeof(int));
    agregar_a_paquete(paqueteMemoria, bufferDeDatosParaMemoria, bytesALeer);
    agregar_a_paquete_solo(paqueteMemoria, &pid, sizeof(int));

    enviar_paquete(paqueteMemoria, socket_memoria);
    eliminar_paquete(paqueteMemoria);

    int tamanio_rta = 0;
    recibir_operacion(socket_memoria);
    void *respuestaMemoria = recibir_buffer(&tamanio_rta, socket_memoria);
    //printf("%s\n",respuestaMemoria);
    
    free(respuestaMemoria);
    free(bufferDeDatosParaMemoria);
    }
    return;
}

//---------------------------------------------Escribir Archivo---------------------------------------------------
//Se deberá solicitar al módulo Memoria la información que se encuentra a partir de la dirección física 
//recibida y se escribirá en el bloque correspondiente del archivo a partir del puntero recibido.
//El tamaño de la información a leer/escribir de la memoria coincidirá con el tamaño del bloque / página.

void escribir_archivo(char *archivo, int puntero, int bytesAEscribir, int marco, int pid){
        
    void *bufferDeDatosDeMemoria = malloc(bytesAEscribir);
    char *path_fcb = path_FCB();
    char *string;
    int tamanio_bloques = tamanio_bloque();
    int cant_bloques_swap = 0;
    int bloquesAEscribir = ceil(bytesAEscribir/tamanio_bloques);
    int primer_bloque = ceil(puntero/tamanio_bloques);
    int bloque_actual;
    int retardo = retardo_acceso_bloques();
    int size;
    
    if(!string_is_empty(archivo)){
        log_info(logger_fs, "Escribir Archivo: %s - Puntero: %d  - Memoria: %d - Tamaño: %d", archivo, puntero, marco, bytesAEscribir);

        // abro el fcb a partir del nombre de archivo del kernel.
        string = string_from_format("%s/%s", path_fcb, archivo);
        t_config *fcb_archivo = iniciar_config(string);
        free(string);

        // leo del fcb
        int bloque_inicial = config_get_int_value(fcb_archivo, "BLOQUE_INICIAL");
        int tamanio_archivo = config_get_int_value(fcb_archivo,"TAMANIO_ARCHIVO");
        int cantidad_bloques = ceil(tamanio_archivo/tamanio_bloques);

        printf("BLOQUES A ESCRIBIR: %d -------------------\n",bloquesAEscribir);

        int tamanio = cantidad_bloques*sizeof(int);
        void *punteros_archivo = malloc(tamanio); 
        get_array_punteros(punteros_archivo,bloque_inicial,cantidad_bloques);
        cant_bloques_swap = cant_bloques_SWAP();

        // RECIBIR PAQUETE DE MEMORIA ver op code
        t_paquete *paqueteMemoria = crear_paquete(LECTURA_MEMORIA);
        agregar_a_paquete_solo(paqueteMemoria, &marco, sizeof(int));
        agregar_a_paquete_solo(paqueteMemoria, &bytesAEscribir, sizeof(int));
        agregar_a_paquete_solo(paqueteMemoria, &pid, sizeof(int));
        enviar_paquete(paqueteMemoria, socket_memoria_servidor);
        eliminar_paquete(paqueteMemoria);

        recibir_operacion(socket_memoria_servidor); //int cod_op = 
        void *bufferAuxiliar = recibir_buffer(&size, socket_memoria_servidor);
        memcpy(bufferDeDatosDeMemoria, bufferAuxiliar, bytesAEscribir);
        
        mem_hexdump((char*)bufferDeDatosDeMemoria, sizeof(int)*4);

        free(bufferAuxiliar);

        
        memcpy(&bloque_actual,punteros_archivo+((primer_bloque)*sizeof(int)),sizeof(int));
        bloque_actual += cant_bloques_swap;
        memcpy(bufferBloques+bloque_actual*tamanio_bloques, bufferDeDatosDeMemoria, tamanio_bloques);
	    msync(bufferBloques+bloque_actual*tamanio_bloques,tamanio_bloques,MS_INVALIDATE);
        log_info(logger_fs, "Acceso Bloque - Archivo: %s - Bloque Archivo: %d - Bloque File System: %d", archivo, primer_bloque, bloque_actual);
        usleep(retardo * 1000);

        free(punteros_archivo);
    } else{
        log_info(logger_fs, "Escribir SWAP - Puntero: %d  - Memoria: %d - Tamaño: %d", puntero, marco, bytesAEscribir);
        
        puntero+=1;
        t_paquete *paqueteMemoria = crear_paquete(LECTURA_MEMORIA);
        agregar_a_paquete_solo(paqueteMemoria, &marco, sizeof(int));
        enviar_paquete(paqueteMemoria, socket_memoria);
        eliminar_paquete(paqueteMemoria);

        recibir_operacion(socket_memoria);
        void *bufferAuxiliar = recibir_buffer(&size, socket_memoria);
        memcpy(bufferDeDatosDeMemoria, bufferAuxiliar, bytesAEscribir); //capaz no va el  + sizeof(int)
        free(bufferAuxiliar);
        memcpy(bufferBloques+puntero*tamanio_bloques, bufferDeDatosDeMemoria, tamanio_bloques);
	    msync(bufferBloques+puntero*tamanio_bloques,tamanio_bloques,MS_INVALIDATE);
        log_info(logger_fs, "Acceso Swap: %d", puntero);
        usleep(retardo * 1000);
    }
    
    free(bufferDeDatosDeMemoria);
    return;
}


//---------------------------------------------Iniciar Proceso---------------------------------------------------
//Iniciar Proceso
//El módulo Memoria solicitará que se reserve una cantidad de bloques de SWAP, como respuesta el módulo 
//File System enviará el listado de los bloques reservados al proceso.
//Al momento de reservar un bloque, el mismo deberá rellenarse con '\0', el algoritmo y 
//las estructuras necesarias para la gestión de estos bloques serán definidas por el grupo.

void iniciar_proceso(int cantidadBloques,int socket_memoria){
    int tam_bloque = tamanio_bloque();
    char* bloque_ocupado[tam_bloque];
    int retardo = retardo_acceso_bloques();

    for(int i = 0; i < tam_bloque;i++){ // Creo un bloque con todos sus elementos en '\0'
        bloque_ocupado[i] = '\0';
    }
    int desplazamiento = 0;
    void *bloquesAsignados = malloc(cantidadBloques * sizeof(int));
    int j = 0;
    while (desplazamiento<cantidadBloques){
        while (bitarray_test_bit(bitmap_swap,j)){ // Busco el bloque libre
            j++;
        }
        memcpy(bloquesAsignados + (desplazamiento*sizeof(int)),&j,sizeof(int)); // Copio el numero de bloque al buffer para memoria
        bitarray_set_bit(bitmap_swap,j); // Marco el bloque como ocupado en el bitmap
        memcpy(bufferBloques + ((j+1)*sizeof(int)),bloque_ocupado,tam_bloque); // Relleno el bloque con '\0'
        log_info(logger_fs, "Acceso Swap: %d",j);
        usleep(retardo * 1000);
        msync(bufferBloques + ((j+1)*sizeof(int)),tam_bloque,MS_INVALIDATE); // Actualizo el archivo en memoria
        j++;
        desplazamiento++;
    }

    t_paquete *paquete_memoria = crear_paquete(BLOQUES_ASIGNADOS);
    agregar_a_paquete(paquete_memoria, bloquesAsignados, cantidadBloques*sizeof(int));

    mem_hexdump((char*)bloquesAsignados, cantidadBloques*sizeof(int));
    enviar_paquete(paquete_memoria,socket_memoria);
    eliminar_paquete(paquete_memoria);
    free(bloquesAsignados);
}

//---------------------------------------------Finalizar Proceso---------------------------------------------------
//Finalizar Proceso
//El módulo Memoria solicitará que se marquen como libres los bloques de 
//SWAP que se envían como parámetro de la solicitud.

void finalizar_proceso(void *punteros,int cant_bloques){
    int i = 0;
    int aux;
    while(i<cant_bloques){
        memcpy(&aux,punteros+(i*sizeof(int)),sizeof(int));
        bitarray_clean_bit(bitmap_swap,aux);
        i++;
    }

}

//----------------------------------------VALORES CONFIG--------------------------------------------------------
int retardo_acceso_FAT(){
    return atoi(config_get_string_value(config_fs,"RETARDO_ACCESO_FAT"));
}

int retardo_acceso_bloques(){
    return atoi(config_get_string_value(config_fs,"RETARDO_ACCESO_BLOQUE"));
}

int tamanio_bloque(){
    return atoi(config_get_string_value(config_fs,"TAM_BLOQUE"));
}

int cant_bloques_SWAP(){
    return atoi(config_get_string_value(config_fs,"CANT_BLOQUES_SWAP"));
}

int cant_bloques_total(){
    return atoi(config_get_string_value(config_fs,"CANT_BLOQUES_TOTAL"));
}

char* path_FCB(){
    return config_get_string_value(config_fs,"PATH_FCB");
}

char* path_bloques(){
    return config_get_string_value(config_fs,"PATH_BLOQUES");
}

char* path_FAT(){
    return config_get_string_value(config_fs,"PATH_FAT");
}

void get_array_punteros(void* punteros_archivo, int bloque_inicial,int cantidad_bloques){
    int retardo_fat = retardo_acceso_FAT();
    int puntero_aux = bloque_inicial;
    int i = 0; 
    // Cargo los indices al array 
    while(i != (cantidad_bloques-1)){
        if(puntero_aux == UINT32_MAX){
            break;
        }
        if (i == 0){
            memcpy(punteros_archivo, &puntero_aux, sizeof(int));
            log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",puntero_aux,*(bufferFAT + (puntero_aux*sizeof(int))));
            memcpy(&puntero_aux,bufferFAT + (puntero_aux*sizeof(int)),sizeof(int));
            usleep(retardo_fat*1000);
            i++;
        }
        memcpy(punteros_archivo+(i*sizeof(int)), &puntero_aux, sizeof(int));
        log_info(logger_fs, "Acceso FAT - Entrada: %d - Valor; %d",puntero_aux,*(bufferFAT + (puntero_aux*sizeof(int))));
        memcpy(&puntero_aux,bufferFAT + (puntero_aux*sizeof(int)),sizeof(int));
        usleep(retardo_fat*1000);
        i++;
    }
}
