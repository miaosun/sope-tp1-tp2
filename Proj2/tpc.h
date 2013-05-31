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
#include <sys/time.h>


typedef struct {
	int n_jogador;
	char nome[20];
	char FIFO_nome[25];

	char *cartas[26];
} Jogador;

typedef struct {
	pthread_mutex_t start_lock;
	pthread_mutex_t log_lock;
	pthread_cond_t var_cond;

	int n_jogadores;
	int vez;
	int senha;
	int ajogar;
	int roundnumber;
	char rondas[27][100];
	char tablecards[52];

	Jogador jogadores_info[52];
} Shared_mem;



//permite que cada jogador escreva, sincronizadamente, no ficheiro log
void *escreve_log(void* arg);

//retorna a data/hora actual
char* getTime();

//thread que abre o fifo de leitura do dealer
void *open_dealer_fifo(void *arg);

//retira uma carta do baralho
char* retira_carta_baralho(int count);

//devolve as cartas de um jogador na forma de uma string
char* apresentacao_cartas(char* c[], int nr);

//thread em que os jogadores esperam que entrem todos os jogadores
void *esperaPorJogadores(void* arg);

//função para jogar uma carta
void jogar_carta(char* mao[], int ncartas);

//funcao para jogar, que sincroniza quem deve jogar quando
void jogar(int nrJogador,char* mao[], int ncartas);

//apresenta o resumo de todas as rondas, caso o utilizador deseje
void* ver_resumo(void *arg);
