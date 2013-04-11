#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

int main(int argc, char* argv[]) {

	if(argc!=4){
		printf("usage: %s dir1 dir2 dt &\n",argv[0]);
		exit(1);
	}

	DIR *dir1, *dir2;

	//abre directório a ser monitorizado
	if ((dir1 = opendir( argv[1])) == NULL) { 
		perror(argv[1]);
		exit(2); 
	}

	//cria directório de backup
	//int mkdir(const char *pathname, mode_t mode);
	if((mkdir(argv[2], S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(argv[2]);
		exit(3);
	}

	//abre directório de backup
	if ((dir2 = opendir( argv[2])) == NULL) { 
		perror(argv[2]);
		exit(2); 
	}

	time_t rawtime;
	struct tm * timeinfo;
	char nome_pasta [20];

	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(nome_pasta,80,"%Y_%m_%d_%H_%M_%S", timeinfo);

	if(chdir(argv[2])==-1) {
		perror(argv[2]);
		exit(4);
	}

	//criar pasta backup
	if((mkdir(nome_pasta, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(nome_pasta);
		exit(4);
	}
	closedir(dir2);

	struct dirent *direntp; 
	struct stat stat_buf;

	chdir(argv[1]);

	while ((direntp = readdir(dir1)) != NULL) 
	{ 
		if (lstat(direntp->d_name, &stat_buf)==-1)
		{
			perror("lstat ERROR");
			exit(3);
		}

		if (S_ISREG(stat_buf.st_mode)) { //regular file
			printf("regular\n");
			//TODO
		}
	}
	
	closedir(dir1);
	
	printf("OK!\n\n");
	exit(0);

}