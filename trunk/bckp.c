#include "bckp.h"


char* dir1;
char* dir2;
DIR *d1, *d2;
int dt;


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

	dir1=argv[1];
	dir2=argv[2];
	dt = atoi(argv[3]);

	//teste utilização
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


	while(1) {
		//abre directório (d1) a ser monitorizado
		if ((d1 = opendir(dir1)) == NULL) { 
			perror(dir1);
			exit(2); 
		}


		// cria pasta backup incremental (YY_MM_DD_HH_MM_SS)
		//TODO se nao usada, tem q ser apagada
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		char nome_pasta [20];
		//cria nome da pasta de backup com base na data&hora actual
		strftime(nome_pasta,80,"%Y_%m_%d_%H_%M_%S", timeinfo);



		//OU USAR SPRINTF!!!
		if(chdir(dir2)==-1) {
			perror(dir2);
			exit(4);
		}
		//criar pasta backup
		if((mkdir(nome_pasta, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
			perror(nome_pasta);
			exit(4);
		}
		//closedir(d2);

		printf("\nPasta Backup: %s\n", nome_pasta);




		//cria ficheiro __bckpinfo__
		char infoPath[PATH_MAX];
		//if((strlen(argv[2])-1)=='/')
		//	sprintf(infoPath, "%s%s/%s", argv[2],nome_pasta, "__bckpinfo__");
		//else
		sprintf(infoPath, "%s/%s/%s", argv[2],nome_pasta, "__bckpinfo__");

		int fd_info;
		if((fd_info=open(infoPath, O_WRONLY | O_CREAT | O_EXCL | O_APPEND, 0644))==-1) {
			perror(infoPath);
			exit(1); //TODO verificar estado de term
		}

		printf("__bckpinfo__ created!\n");




		struct dirent *direntp; 
		struct stat stat_buf;
		chdir(dir1);

		while ((direntp = readdir(d1)) != NULL) 
		{ 
			if (stat(direntp->d_name, &stat_buf)==-1)	{
				perror("stat ERROR");
				exit(3);
			}

			//verifica se se trata de um ficheiro regular
			if (S_ISREG(stat_buf.st_mode)) {

				//cria e escreve em __bckpinfo__
				/*teste erro*///write(fd_info, direntp->d_name, sizeof(dirent->d_name)); //falta /n?
				/*teste erro*///write(infoPath, stat_buf.st_mtime, sizeof(stat_buf.st_mtime));
				/*teste erro*///write(infoPath, dirPath, sizeof(dirPath));
				pid_t pid;
				pid = fork();
				if(pid==0) {
					fileCopy(direntp->d_name, nome_pasta);
					printf("Backup: %s\n",direntp->d_name);
					exit(0); // SAÍDA PODE SER ASSIM?
				}
				else
				{
					int statloc;
					wait(&statloc);
					//QUESTION pai deve esperar?
					//waitpid(pid, &statloc, WNOHANG); //QUESTION usar pid ou -1????
					if(statloc==-1) {
						printf("Processo de copia terminou com erro!");
					}

				}
			}
		}
		closedir(d1);
		sleep(dt);
	}

	printf("Finishing!\n\n");
	return 0;

}