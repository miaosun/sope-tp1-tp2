#include "bckp.h"

char* dir1;
char* dir2;
DIR *d1, *d2;
int dt;
int nExistingChilds = 0;
int receivedSIGUSR1=0;
int nfiles=0;
int nfilesAnt=0;

//t=(aluno *)malloc(sizeof(aluno));

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
	int pid = fork();
	if(pid==-1) {
		perror("Fork Failure!");
	}
	else if(pid==0) {
		printf("Backup: %s\n",filename);

		int fd1, fd2, nr, nw;
		unsigned char buffer[BUFFER_SIZE];

		fd1 = open(filename, O_RDONLY); 
		if (fd1 == -1) {
			perror(filename); 
			exit(12); 
		}

		char newfilePath[PATH_MAX];
		sprintf(newfilePath, "%s/%s/%s", dir2,subdir, filename); //cria pathname do novo ficheiro no subdirectorio de backup

		fd2 = open(newfilePath, O_WRONLY | O_CREAT | O_EXCL, 0644);
		if (fd2 == -1) {
			perror(newfilePath);
			close(fd1);
			exit(13);
		} 
		while ((nr = read(fd1, buffer, BUFFER_SIZE)) > 0) 
			if ((nw = write(fd2, buffer, nr)) <= 0 || nw != nr) { 
				perror(newfilePath);
				close(fd1);
				close(fd2);
				exit(14); 
			}
		close(fd1);
		close(fd2);
		exit(EXIT_SUCCESS);
	}
}

void sigusr_handler(int signo)
{
	printf("SIGUSR1 Received!\n");
	receivedSIGUSR1=1;
}

int main(int argc, char* argv[]) {

	//informação de utilização, caso utilizador não insira todos os parâmetros
	if(argc!=4) {
		printf("usage: %s d1 d2 dt &\n",argv[0]);
		exit(2);
	}

	dir1 = argv[1]; //directório 1 (a fazer backup)
	dir2 = argv[2]; //directorio 2 (destino backup)
	dt = atoi(argv[3]); //dt = intervalo de tempo entre backups

	struct sigaction action; 
	action.sa_handler = sigusr_handler; 
	sigemptyset(&action.sa_mask); 
	action.sa_flags = 0;

	//instala handler para sinal SIGUSR1
	if (sigaction(SIGUSR1,&action,NULL) < 0) { 
		fprintf(stderr,"Unable to install SIGUSR1 handler\n");
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
		exit(4);
	}

	FILE *bckpinfoAnt=NULL; //apontador para ficheiro __bckpinfo__ do backup incremental anterior
	int FirstIteration = 1; //variavel auxiliar indicadora se o processo de backup se encontra na 1ª iteração (full backup)


	while(!receivedSIGUSR1) {

		//abre directório (d1) a ser monitorizado
		if ((d1 = opendir(dir1)) == NULL) { 
			perror(dir1);
			exit(6);
		}

		int auxAlteracao = 0;

		char subdirectory[20];
		createBackupFoldername(subdirectory);
		printf("\nPasta Backup: %s\n", subdirectory);

		char subdirPath[PATH_MAX];
		sprintf(subdirPath, "%s/%s",dir2,subdirectory);
		if((mkdir(subdirPath, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
			perror(subdirectory);
			exit(7);
		}

		char bckpinfoPath[PATH_MAX];
		sprintf(bckpinfoPath, "%s/%s/%s", dir2,subdirectory, "__bckpinfo__");
		FILE *bckpinfo = fopen(bckpinfoPath, "w"); //cria e abre ficheiro __bckpinfo__

		struct dirent *direntp;
		struct stat stat_buf;

		printf("Backing up...\n\n");

		//altera working directory para dir1
		if((chdir(dir1))==-1) {
			perror(dir1);
			exit(5);
		}
		nfilesAnt=nfiles;
		nfiles=0;
		while ((direntp = readdir(d1)) != NULL) 
		{
			if (stat(direntp->d_name, &stat_buf)==-1)	{
				perror("stat ERROR!\n");
				exit(8);
			}

			if (S_ISREG(stat_buf.st_mode)) //verifica se se trata de um ficheiro regular
			{
				nfiles++;
				char *filename= direntp->d_name;
				char *mtime= ctime(&stat_buf.st_mtime);

				if(FirstIteration) {
					fflush(bckpinfo);
					writeTobckpinfo(bckpinfo, filename, mtime, subdirectory); //escreve bkcpinfo
					fflush(bckpinfo);
					fileCopy(filename, subdirectory); //lança processo de cópia
					auxAlteracao=1;
					fflush(bckpinfo);
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
						if((read_bckpinfo(&a1,&a2,&a3, bckpinfoAnt))==-1) {
							break;
						}
						if(strcmp(a1,filename)==0) {
							auxEncontra=1;
							break;
						}
					}

					if(auxEncontra==0) { //ficheiro não existia no backup anterior
						printf("ficheiro n existia\n");
						writeTobckpinfo(bckpinfo, filename, mtime, subdirectory); //escreve em __bckpinfo__
						fflush(bckpinfo);
						fileCopy(filename, subdirectory); //lança processo de cópia
						auxAlteracao=1;
					}
					else
					{
						if(strcmp(a2,mtime)!=0) { //ficheiro já existia mas foi modificado
							printf("ficheiro modificado\n");
							writeTobckpinfo(bckpinfo, filename, mtime, subdirectory); //escreve em __bckpinfo__
							fflush(bckpinfo);
							fileCopy(filename, subdirectory); //lança processo de cópia
							auxAlteracao=1;
						}
						else
						{	//ficheiro já existia e não foi alterado
							printf("ficheiro inalterado - no backed up\n");
							writeTobckpinfo(bckpinfo, filename, a2, a3); //escreve em __bckpinfo__
							fflush(bckpinfo);
						}
					}

					free(a1); free(a2); free(a3); //libertada memória alocada pelos getline's
					rewind(bckpinfoAnt); //apontador do ficheiro é posto a apontar para o inicio do ficheiro
				}
			}
		}

		//verifica se existiu alguma alteração, caso não tenha: apaga directorio
		if(!FirstIteration && nfiles==nfilesAnt && auxAlteracao==0) {
			nExistingChilds++;
			printf("vai fazer rm!\n");
			if((fork())==0) {
				execlp("rm","rm","-R",subdirPath,NULL);
				printf("Erro! Command 'rm' not executed!\n");
				exit(9);
			}
		}
		else {
			//se houve alterações, guarda apontador para ficheiro __bckpinfo__ a ser usado no próximo ciclo
			fclose(bckpinfo);
			bckpinfoAnt = fopen(bckpinfoPath, "r");
		}

		//recebe estados de terminação dos filhos já terminados
		int i=nExistingChilds;
		while(i--) {
			if(waitpid(-1,NULL,WNOHANG) >0)
				nExistingChilds--;
		}

		//void rewinddir(DIR *dir);
		if((closedir(d1))==-1){
			perror(dir1);
			exit(10);
		}

		FirstIteration=0;
		printf("sleep\n\n\n");
		sleep(dt);
	}

	//espera que todos os processos filho terminem
	while(nExistingChilds--) {
		wait(NULL);
	}

	//altera permissoes do directorio de backup apenas para leitura e pesquisa,
	//evitando assim alterações indevidas que poderiam pôr em causa a correcta recuperação dos ficheiros
	chmod(dir2, S_IXUSR|S_IXGRP|S_IXOTH|S_IRUSR|S_IRGRP|S_IROTH);

	fclose(bckpinfoAnt);
	if((closedir(d2))==-1) {
		perror(dir2);
		exit(11);
	}

	printf("Finishing Backup...\n\n");
	return 0;
}
