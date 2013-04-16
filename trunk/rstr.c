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

DIR *d2, *d3, *d4;



char* dir2;
char* dir3;


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

	dir2=argv[1];
	dir3=argv[2];

	//teste utilização
	if(argc!=3){
		printf("usage: %s d1 d2\n",argv[0]);
		exit(1);
	}

	struct dirent **namelist;
	int n, i;
	//abre directório (dir2) onde contem os backups todos e mostrar todos os backups que existem
	if ((d2 = opendir(dir2)) != NULL)
	{
		/*while((dirent = readdir(d2)) != NULL)      // isto aqui mostra conteudos nao por ordem, tipo random
		{
			if(dirent->d_type == DT_DIR)
				if((strcmp(dirent->d_name, ".") != 0) && strcmp(dirent->d_name, "..") != 0)
					printf("%s\n", dirent->d_name);			
		}*/		
		
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
					//free(namelist[i]);
				}
			}
			//free(namelist);
		}
		closedir(d2);
	}
	else
	{
		perror("argv[1]");
		exit(4);
	}

//////////////

	//cria directório de restore (dir3)
	int mkdir(const char *pathname, mode_t mode);
	if((mkdir(dir3, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(dir3);
		exit(3);
	}
	//abre directório de backup (d3)
	if ((d3 = opendir(dir3)) == NULL) { 
		perror(dir3);
		exit(2); 
	}

//////////////////

	char op_dir[BUFFER_SIZE];
	printf("\nWhich restore point (time) ?\n");
	scanf("%s", op_dir);

	for(i=0; i<n; i++)
	{
		if(strcmp(namelist[i]->d_name, op_dir) == 0)
		{
			//chdir(argv[1]);
			struct dirent *direntp; 
			struct stat stat_buf;
			char path1[PATH_MAX];
			sprintf(path1, "%s/%s", dir2,op_dir);
			if ((d4 = opendir(path1)) == NULL) { 
				perror(op_dir);
				exit(2); 
			}

			while ((direntp = readdir(d4)) != NULL) 
			{ 
				printf("testtesttest: %s\n", direntp->d_name);
				if (lstat(direntp->d_name, &stat_buf)==-1)
				{
					perror("lstat ERROR");
					exit(3);
				}
		
				if (S_ISREG(stat_buf.st_mode))
					fileCopy(direntp->d_name, argv[2]);		
			}

			closedir(d4);
			break;
		}
	}
	//closedir(d2);
	free(namelist);


	printf("\nFinishing!\n\n");
	exit(0);

}

