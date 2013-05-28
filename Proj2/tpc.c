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

char* baralho_cartas[52]= {"Ac","2c","3c","4c","5c","6c","7c","8c","9c","10c","Jc","Qc","Kc","Ad","2d","3d","4d","5d","6d","7d","8d","9d","10d","Jd","Qd","Kd","Ah","2h","3h","4h","5h","6h","7h","8h","9h","10h","Jh","Qh","Kh","As","2s","3s","4s","5s","6s","7s","8s","9s","10s","Js","Qs","Ks"};


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


void destroy_shared_memory(Shared_mem *shm, int shm_size)
{
	if (munmap(shm,shm_size) < 0)
	{
		perror("Failure in munmap()");
		exit(EXIT_FAILURE);
	}
	if (shm_unlink(SHM_NAME) < 0)
	{
		perror("Failure in shm_unlink()");
		exit(EXIT_FAILURE);
	}
}

char* retira_carta_baralho() {


}

char* apresentacao_cartas(char* c[]) {

}


void esperaPorJogadores()
{
	pthread_mutex_lock(&shm->start_lock);
	while(shm->n_jogadores != shm->senha)
	{
		printf("...Waiting for more players...\n");
		pthread_cond_wait(&shm->var_cond, &shm->start_lock);
	}
	pthread_mutex_unlock(&shm->start_lock);
}

int main(int argc, char *argv[])
{
	int shmfd, n_jogs;
	int myNrjogador;
	char nome[20];
	char SHM_NAME[20];
	char myFIFO[25];


	if(argc != 4)
	{
		printf("Usage: tpc <player's name> <shm name> <n. players>\n");
		exit(1);
	}

	if(atoi(argv[3]) > 52)
	{
		printf("Numero de Jogadores nao pode ser superior a 52!\n");
		exit(2);
	}

	strcpy(nome, argv[1]);

	//atribui à variavel o nome da memória partilhada
	if(sprintf(SHM_NAME, "/%s", argv[2]) < 0)
	{
		perror("sprintf error");
		exit(1);
	}

	//número de jogadores
	n_jogs = atoi(argv[3]);

	//cria uma regiao de memoria partilhada
	if((shmfd = shm_open(SHM_NAME,O_CREAT|O_RDWR|O_EXCL,0660)) < 0)
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
			printf("FIFO %s created\n", myFIFO);

		//jogador retira o seu número (senha)
		myNrjogador = shm->senha;
		//e incrementa variavel senha
		shm->senha++;

		Jogador jg;
		jg.n_jogador = myNrjogador;
		strcpy(jg.nome, nome);
		strcpy(jg.FIFO_nome, myFIFO);

		shm->jogadores_info[myNrjogador] = jg;

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
		myNrjogador = shm->senha;
		shm->senha++;

		Jogador jg;
		jg.n_jogador = myNrjogador;
		strcpy(jg.nome, nome);
		strcpy(jg.FIFO_nome, myFIFO);

		shm->jogadores_info[myNrjogador] = jg;

		printf("Sou o dealer.\n");
		//printf("Numero de Jogadores: %d\n", shm->senha);

	}

	esperaPorJogadores();

	int nr_cartas_por_jogador = 52/n_jogs;

	int fdr;
	//abrir fifo leitura para todos os jogadores
	if(myNrjogador==0) {
		//chama tread abertura fifo leitura
	}
	else {
		open(myFIFO, O_RDONLY);
	}

	if(myNrjogador==0) { //dealer

		int fdw;
		int i;
		for(i=0;i<n_jogs;i++) {
			fdw=open(shm->jogadores_info[i].FIFO_nome, O_WRONLY);

			char* carta;
			int j;
			for(j=0;j<nr_cartas_por_jogador;j++) {
				carta=retira_carta_baralho();
				write(fdw,carta,strlen(carta));
			}
			close(fdw);
		}
	}


	int k, n;
	char* mao_cartas[nr_cartas_por_jogador];
	for(k=0;k<nr_cartas_por_jogador;k++) {
		n=read(fdr,mao_cartas[k],sizeof(char)*4);
	}
	char* repr = apresentacao_cartas(mao_cartas);


	close(fdr);
	//shm->jogadores_info[myNrjogador].cartas = mao_cartas;
	//printf("Mao de Cartas: ");



	printf("hello\n");
	sleep(20);


	//destroy memory?
	unlink(myFIFO);
	shm_unlink(SHM_NAME);



	return 0;
}
