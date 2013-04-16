#include "bckp.h"


char* dir1;
char* dir2;
DIR *d1, *d2;
int dt;


void fileCopy(char* path, char* nome_pasta) {

			int fd1, fd2, nr, nw;
			unsigned char buffer[BUFFER_SIZE]; 

			fd1 = open(path, O_RDONLY); 
			if (fd1 == -1) {
				perror(dir1); 
				exit(5); 
			}
			
			//cria pathname do ficheiro a copiar
			char filePath[PATH_MAX];
			sprintf(filePath, "%s/%s/%s", dir2,nome_pasta, path);
			
	
			fd2 = open(filePath, O_WRONLY | O_CREAT | O_EXCL, 0644);
			if (fd2 == -1) {
				perror(dir2); 
				close(fd1);
				exit(5);
			} 
			while ((nr = read(fd1, buffer, BUFFER_SIZE)) > 0) 
				if ((nw = write(fd2, buffer, nr)) <= 0 || nw != nr) { 
					perror(dir2); 
					close(fd1); 
					close(fd2); 
					exit(6); 
				}

			/*teste erro*///write(fd_info, direntp->d_name, sizeof(dirent->d_name)); //falta /n?
			/*teste erro*///write(infoPath, stat_buf.st_mtime, sizeof(stat_buf.st_mtime));
			/*teste erro*///write(infoPath, dirPath, sizeof(dirPath));
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

	
	//abre directório (d1) a ser monitorizado
	if ((d1 = opendir( argv[1])) == NULL) { 
		perror(argv[1]);
		exit(2); 
	}
	//cria directório de backup (d2)
	//int mkdir(const char *pathname, mode_t mode);
	if((mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(argv[2]);
		exit(3);
	}
	//abre directório de backup (d2)
	if ((d2 = opendir( argv[2])) == NULL) { 
		perror(argv[2]);
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
	*/

	struct dirent *direntp; 
	struct stat stat_buf;
	chdir(argv[1]);
	while ((direntp = readdir(d1)) != NULL) 
	{ 
		if (lstat(direntp->d_name, &stat_buf)==-1)
		{
			perror("lstat ERROR");
			exit(3);
		}
		
		if (S_ISREG(stat_buf.st_mode)) { //regular file
		/*
		pid_t pid = fork();
		if(pid==0) {
			//copia ficheiro TODO
			fileCopy();
		}
		else {
			//wait status filho TODO
		} */
		// copy file
		fileCopy(direntp->d_name, nome_pasta);	
		// end copy file

		}
	}

	closedir(d1);

	printf("Finishing!\n\n");
	exit(0);

}

