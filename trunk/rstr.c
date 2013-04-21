#include "rstr.h"

DIR *d2, *d3;
char* dir2;
char* dir3;


void fileCopy(char* path1, char* path2) {

	int fd1, fd2, nr, nw;
	unsigned char buffer[BUFFER_SIZE]; 

	//abrir o ficheiro de origin onde quer copiar em mode de leitura
	fd1 = open(path1, O_RDONLY); 
	if (fd1 == -1) {
		perror(path1); 
		exit(5); 
	}			
	//criar e abrir o ficheiro onde vai ser escrito o contéudo do ficheiro do origin
	fd2 = open(path2, O_WRONLY | O_CREAT | O_EXCL, 0644);
	if (fd2 == -1) {
		perror(path2); 
		close(fd1);
		exit(5);
	} 
	//processo de ler do ficheiro original de tamanho BUFFER_SIZE de cada vez para escrever no ficheiro destino
	while ((nr = read(fd1, buffer, BUFFER_SIZE)) > 0) 
	{
		if ((nw = write(fd2, buffer, nr)) <= 0 || nw != nr) { 
			perror(path2); 
			close(fd1); 
			close(fd2); 
			exit(6); 
		}
	}
	//fechar os descritores
	close(fd1); 
	close(fd2); 
}

//lê informação de um ficheiro (nome, data de modificação e pasta backup onde se encontra) do __bckpinfo__
int read_bckpinfo(char **filename, char **datemodi, char **pathname, FILE *fp)
{
	ssize_t read;
	size_t len=0;
	//1+3*n (n=0,1,2...) linha: nome do ficheiro
	if((read = getline(filename, &len, fp)) == -1) {
		return -1;
	}
	if (((*filename)[read-1]) == '\n')
		((*filename)[read-1]) = '\0';

	//2+3*n (n=0,1,2...) linha: data da última alteração do ficheiro
	if((read = getline(datemodi, &len, fp)) == -1) 
		perror("__bckpinfo__-datemodif");

	//3+3*n (n=0,1,2...) linha: nome da pasta onde está o ficheiro da linha 1+3*n
	if((read = getline(pathname, &len, fp)) == -1) 
		perror("__bckpinfo__-pathname");
	if (((*pathname)[read-1]) == '\n')
		((*pathname)[read-1]) = '\0';

	return 0;
}


int main(int argc, char* argv[]) {

	dir2=argv[1];    //directório 1 (a fazer restore)
	dir3=argv[2];    //directorio 2 (destino restore)

	//informação de utilização, caso utilizador não insira numeros dos parâmetros certos
	if(argc!=3){
		printf("usage: %s dir2 dir3\n",argv[0]);
		exit(1);
	}

	struct dirent **namelist;  
	int n, i;
	//abre directório (dir2) onde contem os backups todos e mostrar todos os backups que existem
	if ((d2 = opendir(dir2)) != NULL)
	{
		//procura o diretorio de onde quer fazer restore e guardar todos os nomes dos couteudos na namelist
		n = scandir(dir2, &namelist, NULL, alphasort);
		if(n<0)
			perror("scandir");
		else
		{
			printf("List of avaliable restore points\n");
			for(i=0; i<n; i++)   //imprime todos os restore points
			{
				//para não imprimir "." e ".."
				if((strcmp(namelist[i]->d_name, ".") != 0) && strcmp(namelist[i]->d_name, "..") != 0)
				{
					printf("\t%s\n", namelist[i]->d_name);
				}
				free(namelist[i]);  
			}
			free(namelist);  //libertar a memoria foi colocada para namelist
		}
		closedir(d2);
	}
	else
	{
		perror(dir2);
		exit(4);
	}

	char op_dir[BUFFER_SIZE];
	printf("\nWhich restore point (time) ?\n");


	int b = 1;
	while(b!=0)   //controlo de inserção do nome da pasta que quer fazer restore 
	{
		scanf("%s", op_dir);
		char path1[PATH_MAX];
		sprintf(path1, "%s/%s", dir2,op_dir);			
		b=chdir(path1);
		if(b!=0)
			printf("Restore point doesn't exist, try again!\n");
	}

	//cria directório de restore (dir3)
	if((mkdir(dir3, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH | EEXIST))==-1) {
		perror(dir3);
		exit(3);
	}
	//abre directório de backup (d3)
	if ((d3 = opendir(dir3)) == NULL) { 
		perror(dir3);
		exit(2); 
	}

	FILE *fp;
	char *filename = NULL;
	char *datemodi = NULL;
	char *pathname = NULL;
	//ssize_t read;
	//size_t len = 0;
	char pathorg[PATH_MAX], pathdes[PATH_MAX];

	//abrir o ficheiro __bckpinfo__ em mode de leitura
	fp = fopen("__bckpinfo__", "r");

	if (fp == NULL)
		exit(EXIT_FAILURE);

	int n_filhos = 0;
	while(!feof(fp))
	{
		//ler do ficheiro fp (__bckpinfo__), buscar filename, datemodi e pathname
		if((read_bckpinfo(&filename,&datemodi,&pathname, fp))==-1) {
			break;
		}
		sprintf(pathorg, "%s/%s/%s", dir2, pathname, filename);
		sprintf(pathdes, "%s/%s", dir3, filename);

		pid_t pid;
		pid = fork();
		n_filhos++;
		if(pid == -1)
		{
			fprintf(stderr, "fork error\n");
			exit(EXIT_FAILURE);
		}
		//processo filho
		else if(pid==0) 
		{
			printf("Restore: %s\n",filename);
			fileCopy(pathorg, pathdes);		//processo de copiar ficheiros		
			free(filename);  //libertar memoria colocada para filename
			free(datemodi);  //libertar memoria colocada para datemodi
			free(pathname);  //libertar memoria colocada para pathname
			exit(0); 
		}
		else
		{
			int i = n_filhos;
			while(i--)
			{
				int statloc;
				pid_t p = waitpid(-1, &statloc, WNOHANG);  //receber o estado de terminação de qualquer processo filho sem bloquear
				if(p > 0)
					n_filhos--;
				if(p < 0 || statloc==-1)
					printf("Processo de copia terminou com erro!");
			}

		}
	}
	//libertar memoria alocada para filename, datemodi e pathname
	free(filename);
	free(datemodi);
	free(pathname);

	//receber o estado de terminação de todos os restantes filhos que não foram apanhados pelo waitpid,
	//para não ficar nenhum processo no estado zombie
	while(n_filhos--)
		wait(NULL);

	printf("\nFinishing!\n\n");
	exit(0);

}

