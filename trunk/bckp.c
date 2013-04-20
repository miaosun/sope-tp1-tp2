#include "bckp.h"


char* dir1;
char* dir2;
DIR *d1, *d2;
int dt;
int nExistingChilds = 0;

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
	size_t len=0;
	if((read = getline(a1, &len, bckpinfoAnt)) == -1) {
		perror("__bckpinfo__-filename");
		return -1;
	}
	
	if (((*a1)[read-1]) == '\n')
		((*a1)[read-1]) = '\0';

	if((read = getline(a2, &len, bckpinfoAnt)) == -1) 
		perror("__bckpinfo__-datemodif");
//	if (((*a2)[read-1]) == '\n')
//		((*a2)[read-1]) = '\0';

	if((read = getline(a3, &len, bckpinfoAnt)) == -1) 
		perror("__bckpinfo__-pathname");
	if (((*a3)[read-1]) == '\n')
		((*a3)[read-1]) = '\0';

	return 0;
}


//escreve em __bckpinfo__
void writeTobckpinfo(FILE* file, char* filename, char* mtime, char* subdir) {
	fprintf(file, "%s\n", filename);
	fprintf(file, "%s", mtime);
	fprintf(file, "%s\n", subdir);
}


void fileCopy(char* filename, char* subdir) {

	nExistingChilds++;
	if(fork()==0) {
		printf("Backup: %s\n",filename);

		int fd1, fd2, nr, nw;
		unsigned char buffer[BUFFER_SIZE];

		fd1 = open(filename, O_RDONLY); 
		if (fd1 == -1) {
			perror(filename); 
			exit(5); 
		}

		char newfilePath[PATH_MAX];
		sprintf(newfilePath, "%s/%s/%s", dir2,subdir, filename); //cria pathname do novo ficheiro no subdirectorio de backup

		fd2 = open(newfilePath, O_WRONLY | O_CREAT | O_EXCL, 0644);
		if (fd2 == -1) {
			perror(newfilePath);
			close(fd1);
			exit(5);
		} 
		while ((nr = read(fd1, buffer, BUFFER_SIZE)) > 0) 
			if ((nw = write(fd2, buffer, nr)) <= 0 || nw != nr) { 
				perror(newfilePath);
				close(fd1);
				close(fd2);
				exit(6); 
			}
		close(fd1); 
		close(fd2);

		exit(EXIT_SUCCESS);
	}
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

	//altera directorio corrente para dir1
	if((chdir(dir1))==-1) {
		perror(dir1);
		exit(4);
	}

	while(1) { //TODO !RECEIVEDSIGUSR1 ?


		//abre directório (d1) a ser monitorizado
		if ((d1 = opendir(dir1)) == NULL) { 
			perror(dir1);
			exit(2); 
		}


		//while(1) { //TODO !RECEIVEDSIGUSR1 ?

		int auxAlteracao = 0;

		char subdirectory[20];
		createBackupFoldername(subdirectory);
		printf("\nPasta Backup: %s\n", subdirectory);

		char subdirPath[PATH_MAX];
		sprintf(subdirPath, "%s/%s", argv[2],subdirectory);

		if((mkdir(subdirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
			perror(subdirectory);
			exit(4);
		}

		//cria ficheiro __bckpinfo__
		char bckpinfoPath[PATH_MAX];
		sprintf(bckpinfoPath, "%s/%s/%s", argv[2],subdirectory, "__bckpinfo__");

		FILE *bckpinfo = fopen(bckpinfoPath, "w");

		struct dirent *direntp;
		struct stat stat_buf;


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
//					printf("first iteration\n");
					fileCopy(filename, subdirectory);

					writeTobckpinfo(bckpinfo, filename, mtime, subdirectory); //escreve bkcpinfo
					auxAlteracao=1;
				}
				else
				{
					char *a1=NULL;
					char *a2=NULL;
					char *a3=NULL;

					int auxEncontra = 0; //variavel auxiliar para guardar informação se ficheiro já existia
//					printf("2nd iteration\n");
					// enquanto não ler tdo o ficheiro ou encontrar ficheiro com o mesmo nome
					while(1)
					{
						if((read_bckpinfo(&a1,&a2,&a3, bckpinfoAnt))==-1) {
							printf("ERRO\n");
							break;
						}
//						printf("%s - %s\n",a1, filename);
						if(strcmp(a1,filename)==0) {
							auxEncontra=1;
							break;
						}
					}

					if(auxEncontra==0) { //ficheiro não existia no backup anterior
						printf("ficheiro n existia\n");
						//copia ficheiro!
						fileCopy(filename, subdirectory);
						//escreve em __bckpinfo__
						writeTobckpinfo(bckpinfo, filename, mtime, subdirectory);
						auxAlteracao=1;
					}
					else
					{
						if(strcmp(a2,mtime)!=0) { //ficheiro já existia mas foi modificado
							printf("ficheiro modificado\n");
//							printf("a2 - %s\n", a2);
//							printf("mtime - %s\n", mtime);
							//copia ficheiro!
							fileCopy(filename, subdirectory);
							//escreve em __bckpinfo__
							writeTobckpinfo(bckpinfo, filename, mtime, subdirectory);
							auxAlteracao=1;
						}
						else
						{	//ficheiro já existia e não foi alterado
							//escreve em __bckpinfo__
							printf("ficheiro inalterado - no backued up\n");
							writeTobckpinfo(bckpinfo, filename, a2, a3);
						}
					}

					//liberta memória alocada pelos getline's
					free(a1); free(a2); free(a3);
					//apontador do ficheiro é posto a apontar para o inicio do ficheiro
					rewind(bckpinfoAnt);
				} //no 1st it
			} //IS_REG
		} //while direntp

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
			fclose(bckpinfo);
			bckpinfoAnt = fopen(bckpinfoPath, "r");
		}

		FirstIteration=0;
		printf("sleep\n\n\n");
		sleep(dt);
		//}

		if((closedir(d1))==-1){ //TODO TESTE ERRO? same for chdir
			perror(dir1);
			exit(1);
		}
	}//

	//espera que todos os processos filho terminem
	while(nExistingChilds--) {
		wait(NULL);
	}

	//altera permissoes do directorio de backup apenas para leitura, evitando assim alterações indevidas que poderiam pôr em causa a correcta recuperação dos ficheiros
	chmod(dir2, S_IRUSR|S_IRGRP|S_IROTH);

	printf("Finishing!\n\n");
	return 0;

}