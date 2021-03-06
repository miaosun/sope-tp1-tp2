#include "tpc.h"

#define N_CARTAS 6 //numero de cartas do baralho

char SHM_NAME[20]; //nome da memoria partilhada
Shared_mem *shm;
//tamanho da memoria partilhada (tamanho de uma estrutura Shared_mem)
#define SHM_SIZE sizeof(Shared_mem)

char SEM_NAME[] = "/sem";
char logfilename[30];

int playerNr;
char nome[20];

pthread_t waitThread;

time_t rawtime;
struct tm * timeinfo;
struct timeval tv1;

//baralhos com quantidades diferentes de cartas
//char* baralho_cartas[52]= {"Ac","2c","3c","4c","5c","6c","7c","8c","9c","10c","Jc","Qc","Kc","Ad","2d","3d","4d","5d","6d","7d","8d","9d","10d","Jd","Qd","Kd","Ah","2h","3h","4h","5h","6h","7h","8h","9h","10h","Jh","Qh","Kh","As","2s","3s","4s","5s","6s","7s","8s","9s","10s","Js","Qs","Ks"};
//char* baralho_cartas[20]= {"Ac","2c","3c","Qc","Kc","Ad","2d","3d","Qd","Kc","Ah","2h","3h","Qh","Kh","As","2s","3s","Qs","Ks"};
char* baralho_cartas[6]= {"Ac","2c","3c","Qc","Kc","Ad"};


void *waiting_for_play(void* arg) {

	printf("\n\n\nSeleccione para ver Informações:\n");
	while(1) {
		printf("1-Cartas da Mesa\n2-Mao\n3-Rondas Anteriores\n4-Tempo de Jogo\n");

		char inp[10];
		int esc;
		do{
			printf("Escolha: ");
			scanf("%s",inp);
			esc = atoi(inp);
			fflush(stdin);
		} while(esc > 4 || esc < 1);

		switch(esc) {
		case 1:
			printf("Cartas da Mesa: %s\n",shm->tablecards);
			break;
		case 2:
		{
			printf("Cartas da Mao: %s\n",(char*)arg);
			break;
		}
		case 3:
		{
			printf("\n");
			int i;
			for(i=0;i<shm->roundnumber-1;i++)
				printf("Ronda %d: %s\n",i+1,shm->rondas[i]);
			printf("\n");
			break;
		}
		case 4:
		{
			struct timeval tv2;
			gettimeofday(&tv2, NULL);
			printf("Tempo de jogo: %f\n",((double) (tv2.tv_usec - tv1.tv_usec) / 1000000	+ (double) (tv2.tv_sec - tv1.tv_sec)));
			break;
		}
		}
		printf("\n");
	}
	return NULL;
}


//permite que cada jogador escreva, sincronizadamente, no ficheiro log
void *escreve_log(void* arg) {
	pthread_mutex_lock(&shm->log_lock);
	FILE* logf;
	logf = fopen(logfilename,"a");
	fflush(logf);

	fprintf(logf, "%s", (char*)arg);
	fclose(logf);

	pthread_mutex_unlock(&shm->log_lock);
	return NULL;
}

//retorna a data/hora actual
char* getTime() {
	char *tempo;
	tempo=(char*)malloc(sizeof(char)*30);
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	strftime(tempo,20,"%Y-%m-%d %H:%M:%S", timeinfo);
	return tempo;
}

//thread que abre o fifo de leitura do dealer
void *open_dealer_fifo(void *arg) {
	void *ret;
	int fd = open( (char*) arg , O_RDONLY);
	ret=malloc(sizeof(int));
	*(int *)ret=fd;
	return ret;
}

//retira aleatoriamente uma carta do baralho ainda presente
char* retira_carta_baralho(int count) {

	int n = rand() % count;

	char *carta;
	carta=(char*) malloc(sizeof(char)*4);
	carta=baralho_cartas[n];
	baralho_cartas[n]=baralho_cartas[count-1];

	return carta;
}

//devolve as cartas de um jogador na forma de uma string
char* apresentacao_cartas(char* c[], int nr) {
	char* s=(char*)malloc(sizeof(char)*100);
	strcpy(s, "");
	int i;
	for(i=0; i<nr; i++)
	{
		strcat(s, c[i]);
		if(i!=nr-1)
			strcat(s,"-");
	}
	strcat(s,"\n");
	return s;
}

//thread em que os jogadores esperam que entrem todos os jogadores
void *esperaPorJogadores(void* arg)
{
	pthread_mutex_lock(&shm->start_lock);
	while(shm->n_jogadores != shm->senha)
	{
		printf("...Waiting for more players...\n");
		pthread_cond_wait(&shm->var_cond, &shm->start_lock);
	}
	pthread_mutex_unlock(&shm->start_lock);
	return NULL;
}

//função para jogar uma carta
void jogar_carta(char* mao[], int ncartas) {
	int i;
	struct timeval tv2;
	gettimeofday(&tv2, NULL);

	printf("Tempo de jogo: %f\n",((double) (tv2.tv_usec - tv1.tv_usec) / 1000000	+ (double) (tv2.tv_sec - tv1.tv_sec)));
	printf("Cartas da Mesa: %s\n\n",shm->tablecards);
	printf("Seleccione a carta a jogar (nr carta):\n");
	for(i=1; i<=ncartas;i++) {
		printf("%d: %s\n", i,mao[i-1]);
	}
	char inp[10];
	int esc;
	do{
		printf("Escolha: ");
		scanf("%s",inp);
		esc = atoi(inp);
		fflush(stdin);
	}while(esc > ncartas || esc < 1);

	char carta[4];
	strcpy(carta,mao[esc-1]);
	strcpy(mao[esc-1],mao[ncartas-1]);
	char line1[150];
	char line2[1000];

	sprintf(line1,"%s | Player%d-%s | play           | %s\n",getTime(), playerNr,nome,carta);
	sprintf(line2,"%s%s | Player%d-%s | hand           | %s",line1,getTime(), playerNr,nome,apresentacao_cartas(mao,ncartas-1));

	pthread_t tid;
	//thread que escreve o ficheiro log
	pthread_create(&tid, NULL, escreve_log, (void *)&line2);

	strcat(shm->tablecards,carta);
	if(shm->ajogar<(shm->n_jogadores-1))
		strcat(shm->tablecards,"  -  ");
	shm->vez++;
	if(shm->vez==shm->n_jogadores)
		shm->vez=0;
	shm->ajogar++;

	//permite que os outros jogadores verifiquem se já chegou a sua vez de jogar
	pthread_cond_broadcast(&shm->var_cond);
	char* apr= apresentacao_cartas(mao, ncartas);
	pthread_create(&waitThread,NULL,waiting_for_play,(void *)apr);
}

//funcao para jogar, que sincroniza quem deve jogar quando
void jogar(int nrJogador,char* mao[], int ncartas) {
	pthread_mutex_lock(&shm->start_lock);

	if(nrJogador==0) {

		while(shm->vez != nrJogador) //enquanto nao for a sua vez
		{
			printf("Vez do jogador %d\n",shm->vez);
			pthread_cond_wait(&shm->var_cond, &shm->start_lock);
		}
		//"limpa" a mesa, guarda informação das cartas da mesa, actualiza vez, aumenta o numero da ronda
		if(shm->roundnumber!=0) {
			printf("Fim da Ronda: %s\n\n",shm->tablecards);
			printf("Ronda %d!\n",shm->roundnumber+1);
			strcpy(shm->rondas[shm->roundnumber-1],shm->tablecards);
		}
		shm->vez=0;
		shm->roundnumber++;
		shm->ajogar=0;
		strcpy(shm->tablecards,"");
	} else {
		while(shm->vez != nrJogador) //enquanto não for a sua vez
		{
			printf("É a vez do jogador %d\n",shm->vez);
			pthread_cond_wait(&shm->var_cond, &shm->start_lock);
		}
	}
	pthread_cancel(waitThread); //fecha thread para ver informaçoes, lançada enquanto espera pela sua vez
	printf("É a sua vez de Jogar\n\n");
	jogar_carta(mao,ncartas); //jogar uma carta
	pthread_mutex_unlock(&shm->start_lock);
}

//apresenta o resumo de todas as rondas, caso o utilizador deseje
void* ver_resumo(void *arg)
{
	char inp[10];
	char esc;
	do{
		printf("Escolha: ");
		scanf("%s",inp);
		esc = inp[0];
		fflush(stdin);
	}while(esc!='s' && esc!='n');

	if(esc=='s') {
		int i;
		printf("\n");
		for(i=0;i<N_CARTAS/shm->n_jogadores;i++)
			printf("Ronda %d: %s\n",i+1,shm->rondas[i]);
		printf("\n");
	}
	return NULL;
}


int main(int argc, char *argv[])
{
	int shmfd, n_jogs;
	char myFIFO[25];

	if(argc != 4)
	{
		printf("Usage: tpc <player's name> <shm name> <n. players>\n");
		exit(1);
	}

	if(atoi(argv[3]) > N_CARTAS)
	{
		printf("Numero de Jogadores nao pode ser superior a %d!\n", N_CARTAS);
		exit(2);
	}

	strcpy(nome, argv[1]);

	//atribui à variavel o nome da memória partilhada
	if(sprintf(SHM_NAME, "/%s", argv[2]) < 0)
	{
		perror("sprintf error");
		exit(1);
	}
	if(sprintf(logfilename, "%s.log", argv[2]) < 0)
	{
		perror("sprintf error");
		exit(1);
	}

	n_jogs = atoi(argv[3]); //número de jogadores

	sem_t *sem;
	sem = sem_open(SEM_NAME,O_CREAT,0600,1);
	if(sem==SEM_FAILED) {
		sem=sem_open(SEM_NAME,0,0600,1);
		if(sem==SEM_FAILED) {
			perror("sem_open() failure");
			exit(4);
		}
	}


	//semaforo que nao permite que mais do que um jogador tente criar a memoria partilhada em simultaneo
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
			perror("failure in mmap()");
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
		playerNr = shm->senha;
		//e incrementa variavel senha
		shm->senha++;

		Jogador jg;
		jg.n_jogador = playerNr;
		strcpy(jg.nome, nome);
		strcpy(jg.FIFO_nome, myFIFO);

		shm->jogadores_info[playerNr] = jg;

		printf("Sou o jogador numero: %d\n", playerNr);

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
			perror("failure in mmap()");
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
		playerNr = shm->senha;
		shm->senha++;

		Jogador jg;
		jg.n_jogador = playerNr;
		strcpy(jg.nome, nome);
		strcpy(jg.FIFO_nome, myFIFO);

		shm->jogadores_info[playerNr] = jg;

		shm->roundnumber=0;
		shm->vez=0;
		shm->ajogar=shm->n_jogadores;

		printf("Sou o dealer. Jogador numero 0.\n");

		char line[300];
		char s[100] = "when                | who           | what           | result                         \n";
		sprintf(line,"%s%s | Dealer -%s | deal           | -               \n", s,getTime(), nome);
		pthread_t tid;
		pthread_create(&tid, NULL, escreve_log, (void *)&line);

	}


	sem_post(sem);

	pthread_t tide;
	pthread_create(&tide, NULL, esperaPorJogadores, NULL);
	pthread_join(tide,NULL);


	gettimeofday(&tv1, NULL);

	int nr_cartas_por_jogador = N_CARTAS/n_jogs;

	int fdr;
	//abrir fifo leitura para todos os jogadores
	if(playerNr!=0)
		fdr=open(myFIFO, O_RDONLY);

	if(playerNr==0) { //dealer
		pthread_t tid;
		pthread_create(&tid, NULL,open_dealer_fifo,(void*)myFIFO);
		void *r;

		srand (time(NULL));
		int fdw;
		int count_cartas=N_CARTAS;
		int i;


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

	printf("Cartas da Mao: ");
	char* repr_cartas=apresentacao_cartas(mao_cartas,nr_cartas_por_jogador);
	printf("%s",repr_cartas);
	int nrcartas=nr_cartas_por_jogador;

	char line[300];
	sprintf(line,"%s | Player%d-%s | receive_cards  | %s",getTime(), playerNr,nome,repr_cartas);
	pthread_t tid;
	pthread_create(&tid, NULL, escreve_log, (void *)&line);

	int rounds=N_CARTAS/n_jogs;
	while(rounds>0) {
		jogar(playerNr,mao_cartas,nrcartas);
		rounds--;
		nrcartas--;
	}
	pthread_cancel(waitThread);

	if(playerNr == 0) { //espera que jogo acabe para registar a ultima ronda
		printf("\n...Waiting for others to end...\n\n");
		while(shm->ajogar != shm->n_jogadores){}
		strcpy(shm->rondas[shm->roundnumber-1],shm->tablecards);
	}

	printf("Fim do Jogo!\n");
	struct timeval tv2;
	gettimeofday(&tv2, NULL);
	//apresenta tempo total desde o inicio do jogo
	printf("Tempo total de jogo: %f\n\n\n",((double) (tv2.tv_usec - tv1.tv_usec) / 1000000
			+ (double) (tv2.tv_sec - tv1.tv_sec)));

	printf("Prentede ver resumo de todas as jogadas? (s/n)\n");
	pthread_t tid1;
	pthread_create(&tid1, NULL, ver_resumo, NULL);
	pthread_join(tid1,NULL);


	//fecha FIFO do jogador
	close(fdr);
	//destroi FIFO do jogador
	if(unlink(myFIFO)<0) {
		printf("Erro a destroir FIFO\n");
	}
	if(sem_close(sem)<0) //fecha semaforo
		perror("sem_close failure");

	//munmap da melhoria partilhada
	if (munmap(shm,SHM_SIZE) < 0)
	{
		perror("Failure in munmap()");
		exit(EXIT_FAILURE);
	}
	if(playerNr==0) {
		if(sem_unlink(SEM_NAME)<0) //remove semaforo
			perror("sem_unlink failure");

		//dealer destroi a regiao de memoria partilhada
		while(shm_unlink(SHM_NAME) < 0)
		{
			perror("Waiting for remove the shared memory...\n");
			sleep(2);
		}
	}

	printf("Exiting...\n");
	pthread_exit(0);
}
