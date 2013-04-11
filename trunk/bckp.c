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

	struct tm data_actual;
	char nome_pasta[20];

	//year_month_day_hours_minutes_seconds (example: 2013_03_15_12_30_00)
	sprintf(nome_pasta, "%d_%d_%d_%d_%d_%d", data_actual.tm_year, data_actual.tm_mon, data_actual.tm_mday, data_actual.tm_hour, data_actual.tm_min, data_actual.tm_sec);

	if(chdir(argv[2])==-1) {
		perror(argv[2]);
		exit(4);
	}

	//criar pasta backup
	if((mkdir("nome_pasta", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(nome_pasta);
		exit(4);
	}

	closedir(dir2); 

	printf("OK!\n\n");
	exit(0);

	//chdir(argv[1]);
	//closedir(dirp);

}