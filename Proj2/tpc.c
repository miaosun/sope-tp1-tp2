#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> // For O_* constants
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <pthread.h>

#define SHM_SIZE 10
char *SHM_NAME;

int 

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;  //mutex para a sec. crit.

int main(int argc, char *argv[])
{
	int shmfd, n_jogador;
	char *shm;
	char *nome;

	if(argc != 4)
	{
		printf("Usage: tpc <player's name> <shm name> <n. players>\n");
		return 1;
	}

	nome = argv[1];
	if(sprintf(SHM_NAME, "/%s", argv[2]) < 0)
	{
		perror("sprintf error");
		exit(1);
	}
	n_jogador = atoi(argv[3]);

	//create the shared memory region
	if((shmfd = shm_open(SHM_NAME,O_CREAT|O_RDWR|O_EXCL,0600)) < 0)
	{
		if((shmfd = shm_open(SHM_NAME,O_RDWR,0600)) < 0)
		{
			perror("shm_open error");
			exit(2);
		}
	}

	//attach this region to virtual memory
	shm = (char *) mmap(0,SHM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
	if(shm == MAP_FAILED)
	{
		perror("READER failure in mmap()");
		exit(2);
	}
















	return 0;
}