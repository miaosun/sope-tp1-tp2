#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h> // For O_* constants
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pthread.h>
#include <string.h>
#include <errno.h>


char *SHM_NAME;

typedef struct {
	int n_jogador;
	char nome[20];
	char FIFO_nome[25];

	char *cartas[26];
} Jogador;

typedef struct {
	pthread_mutex_t start_lock;
	pthread_cond_t var_cond;

	int n_jogadores;
	int vez;
	int senha;
	int roundnumber;

	char tablecards[52][4];

	Jogador jogadores_info[52];
} Shared_mem;

Shared_mem *shm;

#define SHM_SIZE sizeof(Shared_mem)

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;  //mutex para a sec. crit.

void esperaPorJogadores()
{
	pthread_mutex_lock(&(shm->start_lock));
	while(shm->n_jogadores != shm->senha)
	{
		printf("...Waiting for more players...\n");
		pthread_cond_wait(&(shm->var_cond), &(shm->start_lock));
	}
	pthread_mutex_unlock(&(shm->start_lock));
}

int main(int argc, char *argv[])
{
	int shmfd, n_jogs;
	int nrjogador;
	char nome[20];
	char SHM_NAME[20];
	char myFIFO[25];


	if(argc != 4)
	{
		printf("Usage: tpc <player's name> <shm name> <n. players>\n");
		return 1;
	}

	strcpy(nome, argv[1]);

	if(sprintf(SHM_NAME, "/%s", argv[2]) < 0)
	{
		perror("sprintf error");
		exit(1);
	}

	n_jogs = atoi(argv[3]);

	//create the shared memory region
	if((shmfd = shm_open(SHM_NAME,O_CREAT|O_RDWR|O_EXCL,0600)) < 0)
	{
		if((shmfd = shm_open(SHM_NAME,O_RDWR,0600)) < 0)
		{
			perror("shm_open error");
			exit(2);
		}

		shm = (Shared_mem *) mmap(0,SHM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
		if(shm == MAP_FAILED)
		{
			perror("READER failure in mmap()");
			exit(2);
		}

		pthread_mutex_lock(&shm->start_lock);

		sprintf(myFIFO, "FIFO_%s", nome);

		if(mkfifo(myFIFO, 0660) < 0)
		{
			if(errno == EEXIST)
				printf("FIFO %s already exists\n", myFIFO);
			else
				printf("Can't create FIFO\n");
		}
		else
			printf("FIFO created\n");

		nrjogador = shm->senha;
		shm->senha++;

		Jogador jg;
		jg.n_jogador = nrjogador;
		strcpy(jg.nome, nome);
		strcpy(jg.FIFO_nome, myFIFO);

		shm->jogadores_info[nrjogador] = jg;

		printf("Numero de Jogadores: %d\n", shm->senha);


		pthread_cond_broadcast(&shm->var_cond);
		pthread_mutex_unlock(&shm->start_lock);

	}

	else
	{
		if(ftruncate(shmfd, SHM_SIZE) < 0)
		{
			perror("ftruncate error");
			exit(3);
		}

		//attach this region to virtual memory
		shm = (Shared_mem *) mmap(0,SHM_SIZE,PROT_READ|PROT_WRITE,MAP_SHARED,shmfd,0);
		if(shm == MAP_FAILED)
		{
			perror("READER failure in mmap()");
			exit(2);
		}

		pthread_mutexattr_t mattr;
		pthread_mutexattr_init(&mattr);
		pthread_mutexattr_setpshared(&mattr, PTHREAD_PROCESS_SHARED);

		pthread_mutex_init(&shm->start_lock, &mattr);

		pthread_condattr_t cattr;
		pthread_condattr_init(&cattr);
		pthread_condattr_setpshared(&cattr, PTHREAD_PROCESS_SHARED);
		pthread_cond_init(&shm->var_cond, &cattr);

		sprintf(myFIFO, "FIFO_%s", nome);

		if(mkfifo(myFIFO, 0660) < 0)
		{
			if(errno == EEXIST)
				printf("FIFO %s already exists\n", myFIFO);
			else
				printf("Can't create FIFO\n");
		}
		else
			printf("FIFO created\n");

		shm->n_jogadores = n_jogs;
		nrjogador = shm->senha;
		shm->senha++;

		Jogador jg;
		jg.n_jogador = nrjogador;
		strcpy(jg.nome, nome);
		strcpy(jg.FIFO_nome, myFIFO);

		shm->jogadores_info[nrjogador] = jg;

		printf("Numero de Jogadores: %d\n", shm->senha);



	}




	esperaPorJogadores();






	printf("hello\n");
	sleep(20);

	unlink(myFIFO);
	shm_unlink(SHM_NAME);



	return 0;
}
