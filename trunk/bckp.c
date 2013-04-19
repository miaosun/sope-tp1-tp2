#include "bckp.h"


char* dir1;
char* dir2;
DIR *d1, *d2;
int dt;

// cria pasta backup incremental (YY_MM_DD_HH_MM_SS)
void createBackupFoldername(char* subdir) {
	time_t rawtime;
	struct tm * timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);
	//cria nome da pasta de backup com base na data&hora actual
	strftime(subdir,20,"%Y_%m_%d_%H_%M_%S", timeinfo);
}

//lê informação de um ficheiro (nome, data de modificação e pasta backup onde se encontra) do __bckpinfo__
int read_bckpinfo(char **a1, char **a2, char **a3, FILE *bckpinfoAnt)
{
	ssize_t read;

	if((read = getline(a1, NULL, bckpinfoAnt)) == -1) {
		//perror("__bckpinfo__-filename");
		return -1;
	}
	if (*a1[read-1] == '\n')
		*a1[read-1] = '\0';

	if((read = getline(a2, NULL, bckpinfoAnt)) == -1) 
		perror("__bckpinfo__-datemodif");
	if (*a2[read-1] == '\n')
		*a2[read-1] = '\0';

	if((read = getline(a3, NULL, bckpinfoAnt)) == -1) 
		perror("__bckpinfo__-pathname");
	if (*a3[read-1] == '\n')
		*a3[read-1] = '\0';

	return 0;
}


//escreve em __bckpinfo__
void writeTobckpinfo(FILE* file, char* filename, char* mtime, char* subdir) {
	fprintf(file, "%s\n", filename);
	fprintf(file, "%s", mtime);
	fprintf(file, "%s\n", subdir);
}

//			pid_t pid = fork();
//			if(pid==0) {
//				fileCopy(direntp->d_name, nome_pasta);
//				printf("Backup: %s\n",direntp->d_name);
//				exit(EXIT_SUCCESS);
//			}
//			else {
//
//				int statloc; //QUESTION pai deve esperar?
//				wait(&statloc); //waitpid(pid, &statloc, WNOHANG); //QUESTION usar pid ou -1????
//				if(statloc==-1) printf("Processo de copia terminou com erro!\n");
//			}
void fileCopy(char* file_name, char* nome_pasta) {

	
	int fd1, fd2, nr, nw;
	unsigned char buffer[BUFFER_SIZE]; 

	fd1 = open(file_name, O_RDONLY); 
	if (fd1 == -1) {
		perror(file_name); 
		exit(5); 
	}

	//cria pathname do ficheiro a copiar
	char filePath[PATH_MAX];
	sprintf(filePath, "%s/%s/%s", dir2,nome_pasta, file_name);

	
	fd2 = open(filePath, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd2 == -1) {
		perror(filePath);
		close(fd1);
		exit(5);
	} 
	while ((nr = read(fd1, buffer, BUFFER_SIZE)) > 0) 
		if ((nw = write(fd2, buffer, nr)) <= 0 || nw != nr) { 
			perror(filePath);
			close(fd1); 
			close(fd2); 
			exit(6); 
		}
	close(fd1); 
	close(fd2); 
}


int main(int argc, char* argv[]) {

	dir1 = argv[1]; //directório 1 (a fazer backup)
	dir2 = argv[2]; //directorio 2 (destino backup)
	dt = atoi(argv[3]); //dt = intervalo de tempo entre backups

	//informação de utilização, caso utilizador não insira todos os parâmetros
	if(argc!=4){
		printf("usage: %s d1 d2 dt &\n",argv[0]);
		exit(1);
	}


	//cria directório de backup (d2)
	//int mkdir(const char *pathname, mode_t mode);
	if((mkdir(dir2, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(dir2);
		exit(3);
	}
	//abre directório de backup (d2)
	if ((d2 = opendir(dir2)) == NULL) { 
		perror(dir2);
		exit(2); 
	}

	FILE *bckpinfoAnt=NULL;
	int FirstIteration = 1;
	int auxAlteracao = 0;
	int nExistingChilds = 0;

	while(1) { //TODO !RECEIVEDSIGUSR1 ?

		//abre directório (d1) a ser monitorizado
		if ((d1 = opendir(dir1)) == NULL) { 
			perror(dir1);
			exit(2); 
		}

		char subdirectory[20];
		createBackupFoldername(subdirectory);
		printf("\nPasta Backup: %s\n", subdirectory);

		//OU USAR SPRINTF!!!
		if(chdir(dir2)==-1) {
			perror(dir2);
			exit(4);
		}
		//criar pasta backup
		if((mkdir(subdirectory, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
			perror(subdirectory);
			exit(4);
		}
		//closedir(d2);

		//cria ficheiro __bckpinfo__
		char bckpinfoPath[PATH_MAX];
		sprintf(bckpinfoPath, "%s/%s/%s", argv[2],subdirectory, "__bckpinfo__");

		//		int fd_info;
		//		if((fd_info=open(bckpinfoPath, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 0644))==-1) {
		//			perror(bckpinfoPath);
		//			exit(1); //TODO verificar estados de term
		//		}
		FILE *bckpinfo = fopen(bckpinfoPath, "w");


		struct dirent *direntp;
		struct stat stat_buf;

		if(chdir(dir1)==-1) {
			perror(dir1);
			exit(4);
		}

		printf("Backing up...\n\n");

		while ((direntp = readdir(d1)) != NULL) 
		{
			if (stat(direntp->d_name, &stat_buf)==-1)	{
				perror("stat ERROR!\n");
				exit(3);
			}


			if (S_ISREG(stat_buf.st_mode)) //verifica se se trata de um ficheiro regular
			{
				char *filename= direntp->d_name;
				char *mtime= ctime(&stat_buf.st_mtime);

				if(FirstIteration) {
					//copia!
					//fileCopy(filename, subdirectory);

					writeTobckpinfo(bckpinfo, filename, mtime, subdirectory); //escreve bkcpinfo
					auxAlteracao=1;
				}
				else
				{
					char *a1=NULL;
					char *a2=NULL;
					char *a3=NULL;

					int auxEncontra = 0; //variavel auxiliar para guardar informação se ficheiro já existia

					// enquanto não ler tdo o ficheiro ou encontrar ficheiro com o mesmo nome
					while(1)
					{
						if((read_bckpinfo(&a1,&a2,&a3, bckpinfoAnt))==-1)
							break;

						if(strcmp(a1,filename)==0) {
							auxEncontra=1;
							break;
						}
					}

					if(auxEncontra==0) {
						//copia ficheiro!

						//escreve em __bckpinfo__
						writeTobckpinfo(bckpinfo, filename, mtime, subdirectory);
						auxAlteracao=1;
					}
					else
					{
						if(strcmp(a2,mtime)!=0) {
							//copia ficheiro!

							//escreve em __bckpinfo__
							writeTobckpinfo(bckpinfo, filename, mtime, subdirectory);
							auxAlteracao=1;
						}
						else
						{	//ficheiro já existia e não foi alterado
							//escreve em __bckpinfo__
							writeTobckpinfo(bckpinfo, filename, a2, a3);
						}
					}

					//liberta memória alocada pelos getline's
					free(a1); free(a2); free(a3);
					//apontador do ficheiro é posto a apontar para o inicio do ficheiro
					rewind(bckpinfoAnt);
				}
			}
		}

		//recebe estados de terminação dos filhos já terminados
		int i=nExistingChilds; //TODO incrementar em forks!!!
		while(i--) {
			if(waitpid(-1,NULL,WNOHANG) >0)
				nExistingChilds--;
		}


		//verifica se existiu alguma alteração, caso não tenha apaga directorio
		if(auxAlteracao==0) {
			//apaga pasta
			printf("vai fazer rm!\n");
			if((fork())==0){
				char subdirPath[PATH_MAX];
				sprintf(subdirPath, "%s/%s", argv[2],subdirectory);
				execlp("rm","rm","-R",subdirPath,NULL);
				//teste? TODO
			}
			else {
				if(waitpid(-1,NULL,WNOHANG) != 0)
					nExistingChilds++;
			}
		}
		else {
			//se houve alterações, guarda apontador para ficheiro __bckpinfo__ a ser usado no próximo ciclo
			printf("close bckpinfo\n");
			fclose(bckpinfo);
			printf("open bckpinfoANT\n");
			bckpinfoAnt = fopen(bckpinfoPath, "r");
		}

		FirstIteration=0;
		closedir(d1);

		sleep(dt);
		printf("after sleep\n");
	}

	//espera que todos os processos filho terminem
	while(nExistingChilds) {
		if(waitpid(-1,NULL,WNOHANG) >0)
			nExistingChilds--;
	}

	printf("Finishing!\n\n");
	return 0;

}