#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <fcntl.h> //open files

#define BUFFER_SIZE 1024
//#define PATH_MAX 

DIR *d2, *d3;

int main(int argc, char* argv[]) {
/*
	dir1=argv[1];
	dir2=argv[2];
	dt = atoi(argv[3]);
*/
	//teste utilização
	if(argc!=3){
		printf("usage: %s d1 d2\n",argv[0]);
		exit(1);
	}

	
	//abre directório (dir2) onde contem os backups todos e mostrar todos os backups que existem
	if ((d2 = opendir( argv[1])) != NULL)
	{
		/*while((dirent = readdir(d2)) != NULL)      // isto aqui mostra conteudos nao por ordem, tipo random
		{
			if(dirent->d_type == DT_DIR)
				if((strcmp(dirent->d_name, ".") != 0) && strcmp(dirent->d_name, "..") != 0)
					printf("%s\n", dirent->d_name);			
		}*/
		struct dirent **namelist;
		int n, i;
		n = scandir(argv[1], &namelist, NULL, alphasort);
		if(n<0)
			perror("scandir");
		else
		{
			for(i=0; i<n; i++)
			{
				if((strcmp(namelist[i]->d_name, ".") != 0) && strcmp(namelist[i]->d_name, "..") != 0)
				{
					printf("%s\n", namelist[i]->d_name);
					free(namelist[i]);
				}
			}
			free(namelist);
		}
		closedir(d2);
	}
	else
	{
		perror("argv[1]");
		exit(4);
	}
	//cria directório de restore (d3)
	//int mkdir(const char *pathname, mode_t mode);
/*	if((mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(argv[2]);
		exit(3);
	}*//*
	//abre directório de backup (d3)
	if ((d3 = opendir( argv[2])) == NULL) { 
		perror(argv[2]);
		exit(2); 
	}*/
	//mostrar todos os backups que existem no directorio dir2


/*


	//OU USAR SPRINTF!!!
	if(chdir(argv[2])==-1) {
		perror(argv[2]);
		exit(4);
	}
	//criar pasta backup
	if((mkdir(nome_pasta, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(nome_pasta);
		exit(4);
	}
	closedir(d2);


	*/

	
/*
	//cria ficheiro __bckpinfo__
	char infoPath[PATH_MAX];
	//if((strlen(argv[2])-1)=='/')
	//	sprintf(infoPath, "%s%s/%s", argv[2],nome_pasta, "__bckpinfo__");
	//else
		sprintf(infoPath, "%s/%s/%s", argv[2],nome_pasta, "__bckpinfo__");

	int fd_info;
	if((fd_info=open(infoPath, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 0644))==-1) {
	perror(infoPath);
	exit();
	}

	}

	closedir(d1);
*/
	printf("\nFinishing!\n\n");
	exit(0);

}

