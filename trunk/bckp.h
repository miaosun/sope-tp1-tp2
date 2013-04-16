#ifndef BCKP_H_
#define BCKP_H_

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h>
#include <wait.h>


#define BUFFER_SIZE 1024

//funcao que efectua a copia (backup) de um ficheiro de nome "file_name" para a pasta "nome_pasta"
void fileCopy(char* file_name, char* nome_pasta);


#endif