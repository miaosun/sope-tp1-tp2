#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>

#define BUFFER_SIZE 1024

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

			// copy file
			int fd1, fd2, nr, nw;

			fd1 = open(direntp->d_name, O_RDONLY); 
			if (fd1 == -1) {
				perror(argv[1]); 
				return 2; 
			}
			fd2 = open(/* pathname */, O_WRONLY | O_CREAT | O_EXCL, 0644); 
			if (fd2 == -1) { 
				perror(argv[2]); 
				close(fd1); 
				return 3; 
			} 
			while ((nr = read(fd1, buffer, BUFFER_SIZE)) > 0) 
				if ((nw = write(fd2, buffer, nr)) <= 0 || nw != nr) { 
					perror(argv[2]); 
					close(fd1); 
					close(fd2); 
					return 4; 
				} 
			close(fd1); 
			close(fd2);

			// end copy file

		}
	}

	closedir(dir1);

	printf("OK!\n\n");
	exit(0);

}