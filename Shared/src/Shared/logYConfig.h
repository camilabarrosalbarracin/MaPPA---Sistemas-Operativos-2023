#ifndef LOGYCONFIG_H
#define LOGYCONFIG_H

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

t_log* iniciar_logger(char* nombreLog, char* proceso);
t_config* iniciar_config(char* archivo);

#endif 