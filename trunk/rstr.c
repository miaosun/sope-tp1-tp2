#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <wait.h>
#include "funcoes.h"
//#define PATH_MAX 

DIR *d2, *d3;

char* dir2;
char* dir3;

int main(int argc, char* argv[]) {

	dir2=argv[1];
	dir3=argv[2];

	//teste utilização
	if(argc!=3){
		printf("usage: %s dir2 dir3\n",argv[0]);
		exit(1);
	}

	struct dirent **namelist;
	int n, i;
	//abre directório (dir2) onde contem os backups todos e mostrar todos os backups que existem
	if ((d2 = opendir(dir2)) != NULL)
	{
		n = scandir(dir2, &namelist, NULL, alphasort);
		if(n<0)
			perror("scandir");
		else
		{
			printf("List of avaliable restore points\n");
			for(i=0; i<n; i++)
			{
				if((strcmp(namelist[i]->d_name, ".") != 0) && strcmp(namelist[i]->d_name, "..") != 0)
				{
					printf("\t%s\n", namelist[i]->d_name);
				}
			}
		}
		closedir(d2);
	}
	else
	{
		perror(dir2);
		exit(4);
	}


	//cria directório de restore (dir3)
	//int mkdir(const char *pathname, mode_t mode);
	if((mkdir(dir3, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(dir3);
		exit(3);
	}
	//abre directório de backup (d3)
	if ((d3 = opendir(dir3)) == NULL) { 
		perror(dir3);
		exit(2); 
	}
	

	char op_dir[BUFFER_SIZE];
	printf("\nWhich restore point (time) ?\n");
	scanf("%s", op_dir);

	for(i=0; i<n; i++)
	{
		if(strcmp(namelist[i]->d_name, op_dir) == 0)
		{			
			char path1[PATH_MAX], pathorg[PATH_MAX], pathdes[PATH_MAX];
			sprintf(path1, "%s/%s", dir2,op_dir);			
			chdir(path1);

			FILE *fp;
			char *filename = NULL;
			char *datemodi = NULL;
			char *pathname = NULL;
			ssize_t read;
			size_t len = 0;
			
			fp = fopen("__bckpinfo__", "r");
			
			if (fp == NULL)
				exit(EXIT_FAILURE);

			while(!feof(fp))
			{			
				if((read = getline(&filename, &len, fp)) == -1) {
					perror("__bckpinfo__-filename");
				}
				if (filename[read-1] == '\n')
					filename[read-1] = '\0';
				if((getline(&datemodi, &len, fp)) == -1) {
					perror("__bckpinfo__-datemodif");
				}
				if((read = getline(&pathname, &len, fp)) == -1) {
					perror("__bckpinfo__-pathname");
				}
				if (pathname[read-1] == '\n')
					pathname[read-1] = '\0';

				sprintf(pathorg, "%s/%s/%s", dir2, pathname, filename);
				sprintf(pathdes, "%s/%s", dir3, filename);

				pid_t pid;
				pid = fork();
				if(pid==0) 
				{
					printf("Restore: %s\n",filename);
					fileCopy(pathorg, pathdes);				
					exit(0); 
				}
				else
				{
					int statloc;
					wait(&statloc);
					if(statloc==-1)
						printf("Processo de copia terminou com erro!");
				}
			}
			break;
		}
	}
	free(namelist);
	closedir(d3);

	printf("\nFinishing!\n\n");
	exit(0);

}

