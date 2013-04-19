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


// cria pasta backup incremental (YY_MM_DD_HH_MM_SS)
void createBackupFoldername(char* subdir);

//lê informação de um ficheiro (nome, data de modificação e pasta backup onde se encontra) do __bckpinfo__
int read_bckpinfo(char **a1, char **a2, char **a3, FILE *bckpinfoAnt);

//escreve em __bckpinfo__
void writeToInfoFile(FILE* file, char* nm, char* mtime, char* subdir);

//funcao que efectua a copia (backup) de um ficheiro de nome "file_name" para a pasta "nome_pasta"
void fileCopy(char* file_name, char* nome_pasta);


#endif