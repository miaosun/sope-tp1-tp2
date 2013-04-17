#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> //open files

#define BUFFER_SIZE 1024

void fileCopy(char* path1, char* path2) {

			int fd1, fd2, nr, nw;
			unsigned char buffer[BUFFER_SIZE]; 

			fd1 = open(path1, O_RDONLY); 
			if (fd1 == -1) {
				perror(path1); 
				exit(5); 
			}
			
			//cria pathname do ficheiro a copiar
			char filePath[PATH_MAX];
			sprintf(filePath, "%s/%s", path2,path1);
			
	
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

			/*teste erro*///write(fd_info, direntp->d_name, sizeof(dirent->d_name)); //falta /n?
			/*teste erro*///write(infoPath, stat_buf.st_mtime, sizeof(stat_buf.st_mtime));
			/*teste erro*///write(infoPath, dirPath, sizeof(dirPath));
			close(fd1); 
			close(fd2); 

}