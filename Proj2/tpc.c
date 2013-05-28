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
char SEM_NAME[] = "/sem";

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
	int ajogar;
	int roundnumber;
	char* rondas[27];
	char tablecards[52];

	Jogador jogadores_info[52];
} Shared_mem;

Shared_mem *shm;

#define SHM_SIZE sizeof(Shared_mem)

pthread_mutex_t mut = PTHREAD_MUTEX_INITIALIZER;  //mutex para a sec. crit.


//void destroy_shared_memory(Shared_mem *shm)
//{
//	if (munmap(shm,SHM_SIZE) < 0)
//	{
//		perror("Failure in munmap()");
//		exit(EXIT_FAILURE);
//	}
//	if (shm_unlink(SHM_NAME) < 0)
//	{
//		perror("Failure in shm_unlink()");
//		exit(EXIT_FAILURE);
//	}
//}

void *open_dealer_fifo(void *arg) {
	void *ret;
	int fd = open( (char*) arg , O_RDONLY);
	ret=malloc(sizeof(int));
	*(int *)ret=fd;
	return ret;
}

char* retira_carta_baralho(int count) {

	int n = rand() % count;

	char *carta;
	carta=(char*) malloc(sizeof(char)*4);
	carta=baralho_cartas[n];
	baralho_cartas[n]=baralho_cartas[count-1];

	return carta;
}

void apresentacao_cartas(char* c[], int nr) {

	int i;
	for(i=0; i<nr; i++)
	{
		printf("%s", c[i]);
		if(i!=nr-1)
			printf("-");
	}
	printf("\n");
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

void jogar_carta(char* mao[], int ncartas) {
	int i;
	printf("Cartas da Mesa: %s\n\n",shm->tablecards);
	printf("Seleccione a carta a jogar (nr carta):\n");
	for(i=1; i<=ncartas;i++) {
		printf("%d: %s\n", i,mao[i-1]);
	}
	int esc;
	printf("Escolha: ");
	scanf("%d",&esc);

	char carta[4];
	strcpy(carta,mao[esc-1]);
	strcpy(mao[esc-1],mao[ncartas-1]);

	strcat(shm->tablecards,carta);
	if(shm->ajogar<(shm->n_jogadores-1))
		strcat(shm->tablecards,"  -  ");
	shm->vez++;
	if(shm->vez==shm->n_jogadores)
		shm->vez=0;
	shm->ajogar++;

	pthread_cond_broadcast(&shm->var_cond);
}

void jogar(int nrJogador,char* mao[], int ncartas) {
	pthread_mutex_lock(&shm->start_lock);

	if(nrJogador==0) {

		while(shm->vez != nrJogador)
		{
			printf("Vez do jogador %d\n",shm->vez);
			printf("Ver alguma coisa quando outros a jogar?\n");
			printf("1. sldfsdjkf\n");

			//char c;
			//scanf("%c", &c);

			pthread_cond_wait(&shm->var_cond, &shm->start_lock);
		}
		if(shm->roundnumber!=0) {
			printf("Fim da Ronda: %s\n\n",shm->tablecards);
			printf("Nova Ronda!\n");
			shm->rondas[shm->roundnumber] = (char*) malloc(sizeof(shm->tablecards));
			strcpy(shm->rondas[shm->roundnumber],shm->tablecards);
		}
		shm->vez=0;

		shm->roundnumber++;
		shm->ajogar=0;
		strcpy(shm->tablecards,"");

		printf("É a sua vez de Jogar\n\n");
		jogar_carta(mao,ncartas);
		pthread_mutex_unlock(&shm->start_lock);
	} else {

		while(shm->vez != nrJogador)
		{
			printf("É a vez do jogador %d\n",shm->vez);
			pthread_cond_wait(&shm->var_cond, &shm->start_lock);
		}
		printf("É a sua vez de Jogar\n\n");
		jogar_carta(mao,ncartas);
		pthread_mutex_unlock(&shm->start_lock);
	}
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

	sem_t *sem;
	sem = sem_open(SEM_NAME,O_CREAT,0600,1);
	if(sem==SEM_FAILED) {
		sem=sem_open(SEM_NAME,0,0600,1);
		if(sem==SEM_FAILED) {
			perror("sem_open() failure");
			exit(4);
		}
	}

	sem_wait(sem);

	//tenta criar regiao de memoria partilhada
	if((shmfd = shm_open(SHM_NAME,O_CREAT|O_RDWR|O_EXCL,0660)) < 0)
	{
		//falha, significa que já foi criada. agora apenas abre
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

		printf("Sou o jogador numero: %d\n", myNrjogador);

		pthread_cond_broadcast(&shm->var_cond);
		pthread_mutex_unlock(&shm->start_lock);

	}

	else //DEALER
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
		}	//reinicia ronda
		else
			printf("FIFO created\n");

		shm->senha=0;
		shm->n_jogadores = n_jogs;
		myNrjogador = shm->senha;
		shm->senha++;

		Jogador jg;
		jg.n_jogador = myNrjogador;
		strcpy(jg.nome, nome);
		strcpy(jg.FIFO_nome, myFIFO);

		shm->jogadores_info[myNrjogador] = jg;

		shm->roundnumber=0;
		shm->vez=0;
		shm->ajogar=shm->n_jogadores;

		printf("Sou o dealer. Jogador numero 0.\n");

	}

	sem_post(sem);
	esperaPorJogadores();

	int nr_cartas_por_jogador = 52/n_jogs;

	int fdr;
	//abrir fifo leitura para todos os jogadores
	if(myNrjogador!=0)
		fdr=open(myFIFO, O_RDONLY);

	if(myNrjogador==0) { //dealer
		pthread_t tid;
		pthread_create(&tid, NULL,open_dealer_fifo,(void*)myFIFO);
		void *r;

		srand (time(NULL));
		int fdw;
		int count_cartas=52;
		int i;

		FILE *f;
		char filename[20];

		sprintf(filename, "%s.log", argv[2]);

		if((f = fopen(filename,  "w")) ==NULL)
		{
			perror("can't create log file\n");
			exit(5);
		}

		char tempo[];
		time_t rawtime;
		struct tm * timeinfo;
		time(&rawtime);
		timeinfo = localtime(&rawtime);
		//cria nome da pasta de backup com base na data&hora actual
		strftime(tempo,20,"%Y_%m_%d_%H_%M_%S", timeinfo);

		char line[];
		fprintf(f, "when           | who            | what           | result                         \n");
		fprintf(f, "%s | Dealer-%s | deal     | -               ", tempo, nome);
		pthread_t tid;
		sprintf(line, "%s | Dealer-%s | deal     | -               ", tempo, nome);
		pthread_create(&tid, NULL, escreve_log, &line);

		for(i=0;i<n_jogs;i++) {
			fdw=open(shm->jogadores_info[i].FIFO_nome, O_WRONLY);

			char* carta;
			int j;
			for(j=0;j<nr_cartas_por_jogador;j++) {
				carta=retira_carta_baralho(count_cartas);
				count_cartas--;
				write(fdw,carta,sizeof(carta));
			}
			close(fdw);
		}

		pthread_join(tid,&r);
		fdr=*(int *)r;
		free(r);
	}

	int k, n;
	char* mao_cartas[nr_cartas_por_jogador];
	for(k=0;k<nr_cartas_por_jogador;k++) {
		mao_cartas[k]= malloc(sizeof(char)*4);
		n=read(fdr,mao_cartas[k],sizeof(char*));
		if(n==0) {
			printf("FIFO failure");
			exit(3);
		}
	}

	printf("Cartas da Mao: \n");
	apresentacao_cartas(mao_cartas,nr_cartas_por_jogador);
	int nrcartas=nr_cartas_por_jogador;

	int rounds=52/n_jogs;
	while(rounds>0) {
		jogar(myNrjogador,mao_cartas,nrcartas);
		rounds--;
		nrcartas--;
	}







	//fecha FIFO do jogador
	close(fdr);
	//destroi FIFO do jogador
	if(unlink(myFIFO)<0) {
		printf("Erro a destroir FIFO\n");
	}
	if(sem_close(sem)<0)
		perror("sem_close failure");

	//munmap da melhoria partilhada
	if (munmap(shm,SHM_SIZE) < 0)
	{
		perror("Failure in munmap()");
		exit(EXIT_FAILURE);
	}
	if(myNrjogador==0) {
		if(sem_unlink(SEM_NAME)<0)
			perror("sem_unlink failure");

		//dealer destroi a regiao de memoria partilhada
		if (shm_unlink(SHM_NAME) < 0)
		{
			perror("Failure in shm_unlink()");
			exit(EXIT_FAILURE);
		}
	}



	printf("Exiting...\n");
	return 0;
}
