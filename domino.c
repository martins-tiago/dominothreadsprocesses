//
// Created by arpinto on 07-10-2016.
//

#include "domino.h"
#include <time.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <semaphore.h>


BLOCK *phand;
HAND *hand;
HAND *handaux;
SEQUENCE *seq;
ALLSEQUENCES *all_sequences;
ALLSEQUENCES *arrayglobal;

int count = 0;
int countPrint = 0;
int destination = 0;

/** Prod Cons **/
int N = 0;
int M = 0;

int prodptr=0;
int consptr=0;

pthread_mutex_t trinco_p=PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t trinco_c=PTHREAD_MUTEX_INITIALIZER;

sem_t pode_prod;
sem_t pode_cons;

/** -------- **/

int main(int argc, const char * argv[]){


    srand((unsigned)time(NULL));

    time_t t = time(NULL);
    struct tm *tm = localtime(&t);
    char fname[64];
    strftime(fname, sizeof(fname), "%a %b %d %Hh %Y", tm);

    char path_save_file[150] = "./data/"; //Caminho para a pasta de dados.
    char *path_file_hand = "./data/mao14P20832S.txt";

    strcat(path_save_file,fname);
    strcat(path_save_file,".txt");
    strcat(path_save_file,"\0");

    //Baralho de peças
    DECK *deck = NULL;
    deck = (DECK *) malloc(sizeof(DECK));
    deck->pfirst = NULL;
    deck->num_blocks_available = NUM_BLOCKS;

    //Inicialização do baralho
    deck->pfirst = create_deck(deck->pfirst);

    //Mão do jogador

    hand = (HAND *) malloc(sizeof(HAND));
    hand->pfirst=NULL;
    hand->hand_size=0;

    //hand = create_hand(hand,deck,10);
    hand = read_hand_from_file(hand, path_file_hand);
    //print_hand(hand);

    //Lista de sequência de peças

    seq = (SEQUENCE *) malloc(sizeof(SEQUENCE));
    //seq->pfirst = NULL;
    //seq->size_of_sequence = 0;

    //Lista com todas as sequências guardadas

    /*all_sequences = (ALLSEQUENCES *) malloc(sizeof(ALLSEQUENCES));
    all_sequences->pfirst = NULL;
    all_sequences->path_file = (char *) malloc(sizeof(char)*150);
    strcpy(all_sequences->path_file, path_save_file);
    all_sequences->number_of_sequences = 0;*/

    /**concatenar e enviar o caminho do fich. para o save_sequences_file()*/
    char fnameaux[50]="";
    strcat(fnameaux,"./data/");
    strcat(fnameaux,fname);
    strcat(fnameaux,".txt");

    destination = open(fnameaux, O_CREAT | O_APPEND | O_WRONLY, 0666);

    if (destination == -1) {
     perror ("Opening Destination File");
     exit(1);
     }

    int i=0;


    /**-------------- Alocar espa�o para arrayGlobal  -------------- **/
    arrayglobal = (ALLSEQUENCES *) malloc(sizeof(ALLSEQUENCES)*hand->hand_size);


    /** -------------- ALOCAR ESPA�O HANDAUX , LE DO FICHEIRO/ALOCA BLOCOS E GUARDA EM HANDAUX -------------- **/
    handaux = (HAND *) malloc(sizeof(HAND)*hand->hand_size);
    handaux->pfirst=NULL;
    handaux->hand_size=0;


    BLOCK * handauxblock=NULL;
    phand=hand->pfirst;

    for(i=0;i<hand->hand_size;i++)
    {
        /**-------------- Alocar espa�o para sequencia -------------- **/

        //ciclo para por a null
        (arrayglobal+i)->pfirst = NULL;

        (arrayglobal+i)->path_file = (char *) malloc(sizeof(char)*150);

        //por o caminho como global
        strcpy((arrayglobal+i)->path_file, path_save_file);
        (arrayglobal+i)->number_of_sequences = 0;
        /** ----------------------------------------------------------------------**/

        /**read a file e faz malloc blocos **/
        *(handaux+i)=*read_hand_from_file((handaux+i), path_file_hand);
        (handaux+i)->hand_size=hand->hand_size;

        /**apontador para o primeiro bloco da mao que alocamos no handaux**/
        handauxblock=(handaux+i)->pfirst;


        /**ciclo para copiar as pecas do hand original rodado para o (handaux+i) � sempre 2|5-5|5....**/
            while(phand!=NULL){

                handauxblock->left_side=phand->left_side;
                handauxblock->right_side=phand->right_side;
                handauxblock=handauxblock->pnext;
                phand=phand->pnext;

            }

            /**roda mao**/
            rodar_pecas(hand);

            phand=hand->pfirst;

    }

    /** Produtor Consumidor **/
    N = hand->hand_size;
    M = 2;
    sem_init(&pode_prod,0,N);
	sem_init(&pode_cons,0,0);

	pthread_t prod[N];
	pthread_t cons[M];

    for(i=0;i<M;i++){

        pthread_create(&cons[i], NULL, &consumidor, NULL);

    }


    for(i=0;i<N;i++)
    {

        pthread_create (&prod[i], NULL, &produtor,(void *)(intptr_t)i); //pre_recursivo

    }

     /** ------------------- **/

    //JOINS
    for(i=0;i<N;i++){

        pthread_join(prod[i], NULL);

    }

    for(i=0;i<M;i++){

        pthread_join(cons[i], NULL);

    }

    return 0;

}
/** Rodar pe�as **/
void rodar_pecas(HAND *hand){

        /**Troca de peca*/
        BLOCK * paux=NULL;
        BLOCK *paux2=NULL;
        paux=hand->pfirst;
        paux2=hand->pfirst;

        while(paux->pnext!=NULL){

            paux=paux->pnext;

        }

        /**primeira vez cabeca disponivel(1)*/

        hand->pfirst->available=1;

        /**roda, coloca a ultima no inicio*/

        hand->pfirst=paux;
        hand->pfirst->pnext=paux2;
        paux->pprev->pnext=NULL;

}

/** Pre recursive backtrack **/

void *produtor(void* i){

    sem_wait(&pode_prod);
    pthread_mutex_lock(&trinco_p);

        BLOCK * paux=NULL;
        int pos=(intptr_t)i;

        paux=(handaux+pos)->pfirst;

        //coloca cabeca indisponivel para o backtrack
        seq->pfirst->left_side=paux->left_side;
        seq->pfirst->right_side=paux->right_side;
        seq->pfirst->available = 1;
        seq->size_of_sequence = 1;
        paux->available=0;

        /*while(paux!=NULL)
        {

            printf("[%d] %d|%d \n",pos,paux->left_side,paux->right_side);
            paux=paux->pnext;

        }*/

        recursive_backtrack(handaux+pos,seq,arrayglobal+pos,1);

    pthread_mutex_unlock(&trinco_p);
    sem_post(&pode_cons);

    return NULL;

}

void * consumidor()
{
        sem_wait(&pode_cons);
        pthread_mutex_lock(&trinco_c);
            int i = 0;
            for(i = 0;i<hand->hand_size; i++)
            {
                print_sequences(arrayglobal+i);
                save_sequences_file(arrayglobal+i);

            }

        pthread_mutex_unlock(&trinco_c);
        sem_post(&pode_prod);
        printf("Consumiu\n");


    return 0;
}



/**
 * create_deck - Função responsável por criar o baralho de peças do domino.
 * @param pblock - Apontador para uma lista de peças inicialmente vazia.
 * @return lista de peças do domino.
 */
BLOCK *create_deck(BLOCK *pblock) {
    short i = 0, valMax = 6, valMin = 0, blocks = NUM_BLOCKS;
    for (valMax = 6; valMax >= 0; valMax--) {
        valMin = 0;
        for (i = 0; i <= valMax; i++) {
            pblock = insert(pblock, valMax, valMin, 1);
            blocks--;
            valMin++;
        }
    }
    return pblock;
}

/**
 * create_hand - Função responsável por distribuir peças do baralho para a mão do jogador.
 * @param phand - Apontador para a mão do jogador.
 * @param pdeck - Apontador para o baralho de peças.
 * @param num_blocks_hand - Número de peças a serem alocada �  mão do jogador.
 * @return Apontador para a primeira peça da mão do jogador.
 */
HAND *create_hand(HAND *phand, DECK *pdeck, short num_blocks_hand) {
    int index = 0;
    short i = 0, upLim = pdeck->num_blocks_available;
    HAND *pprim = phand;
    BLOCK *paux = NULL;
    for (i = 0; i < num_blocks_hand; i++) {
        index = uniform_index_block(0, upLim);
        paux = get_block(pdeck, index);
        phand->pfirst = insert(phand->pfirst, paux->left_side, paux->right_side, paux->available);
        upLim--;
        pdeck->num_blocks_available = upLim;
    }
    pprim->hand_size = num_blocks_hand;
    return pprim;
}

/**
 * uniform_index_block - Gera um valor aleatório entre um intervalo de valores.
 * @param val_min - valor mais baixo do intervalo.
 * @param val_max - valor mais alto do intervalo.
 * @return valor gerado.
 */
int uniform_index_block(short val_min, short val_max) {
    return val_min + rand() % (val_max - val_min + 1);
}

/**
 * get_block - devolve uma peça.
 * @param pdeck - Apontador para o baralho de peças.
 * @param index - Posição da peça a returnar.
 * @return Uma peça do baralho.
 */
BLOCK *get_block(DECK *pdeck, int index) {

    BLOCK *pblock = pdeck->pfirst;
    BLOCK *paux = NULL, *pant = NULL;
    int i = 0;

    if (pblock == NULL) {
        printf("Empty\n");
        return NULL;
    }
    paux = pblock;

    while (paux != NULL && i != index) {
        pant = paux;
        paux = paux->pnext;
        i++;
    }

    if (pant == NULL) {
        pdeck->pfirst = paux->pnext;
        paux->pnext = NULL;
        return paux;
    }

    if (paux != NULL) {
        pant->pnext = paux->pnext;
        paux->pnext = NULL;
        return paux;
    }
    return paux;
}

/**
 * read_hand_from_file - Cria a mão de peças do jogador atrevés da leitura de um ficheiro.
 * @param phand - Apontador para a mão do jogador.
 * @param pathFile - Caminho para o ficheiro.
 * @return Apontador para a primeira peça da mão do jogador.
 */
HAND *read_hand_from_file(HAND *phand, char *pathFile) {
    short i = 0, num_blocks = 0, leftSide = 0, rightSide = 0;;
    HAND *pprim = phand;
    FILE *fp = NULL;
    fp = fopen(pathFile, "r");
    if (fp != NULL) {
        fscanf(fp, "%hi", &num_blocks);
        for (i = 0; i < num_blocks; i++) {
            fscanf(fp, "%*c %*c %hi %*s %hi %*c", &leftSide, &rightSide);
            phand->pfirst = insert(phand->pfirst, leftSide, rightSide, 1);
        }
    }
    fclose(fp);
    pprim->hand_size = num_blocks;
    return pprim;
}

/**
 * print_hand - Imprime a mão do jogador.
 * @param phand - Apontador para a mão do jogador.
 */
void print_hand(HAND *phand)
{
    HAND *paux = phand;
    BLOCK *pblock = paux->pfirst;
    while (pblock != NULL) {
        printf("[%d %d]\n", pblock->left_side, pblock->right_side);
        pblock = pblock->pnext;
    }
}

/**
 * insert - Aloca memória para uma peça e inseri-a num lista de peças.
 * @param pblock - Apontador para uma lista de peças.
 * @param left_side - Valor esquerdo da peça.
 * @param right_side - Valor direito da peça.
 * @param avaiable - Valor que indica se a peça esta dispinivel. (0/1)
 * @return Apontador para uma lista de peças.
 */
BLOCK *insert(BLOCK *pblock, short left_side, short right_side, short avaiable) {

    BLOCK *pnew = NULL;
    BLOCK *paux = pblock;

    pnew = (BLOCK *) malloc(sizeof(BLOCK));
    pnew->left_side = left_side;
    pnew->right_side = right_side;
    pnew->available = avaiable;
    pnew->pnext = pnew;
    pnew->pprev = pnew;

    if (paux == NULL) {
        return pnew;
    }

    paux->pprev->pnext = pnew;
    pnew->pprev = paux->pprev;
    paux->pprev = pnew;
    pnew->pnext = NULL;

    return paux;
}

/**
 * recursive_backtrack - Função recursiva responsável pela pesquisa exaustiva de sequências de peças.
 * @param phand - Apontador para a mão do jogador.
 * @param pseq - Apontador para a primeira peça da lista de sequência.
 * @param pall_sequences - Apontador para uma lista de todas as sequências geradas.
 * @param inserted - Número de peças inseridas na sequencia.
 * @return 0 - Quando termina a procura.
 */
int recursive_backtrack(HAND *phand, SEQUENCE *pseq, ALLSEQUENCES *pall_sequences, short inserted) {


    int i = 0;
    BLOCK *block = NULL;
    block = phand->pfirst;

    for (i = 0; i < phand->hand_size; i++) {
        if (block->available == 1) {
            if (is_current_assignment_consistent(pseq, block, inserted) == ASSIGNMENT_CONSISTENT) {

                BLOCK *pnew = (BLOCK *) malloc(sizeof(BLOCK));
                pnew->left_side = block->left_side;
                pnew->right_side = block->right_side;
                pnew->available = 0;
                pnew->pnext = NULL;
                pnew->pprev = NULL;

                if(inserted == 0) {
                    pseq->pfirst = pnew;
                    pseq->pfirst->pnext = NULL;
                    pseq->pfirst->pprev = pnew;
                }else{
                    pnew->pprev = pseq->pfirst->pprev;
                    pnew->pprev->pnext = pnew;
                    pnew->pnext = NULL;
                    pseq->pfirst->pprev = pnew;
                }

                inserted++;
                pseq->size_of_sequence++;
                block->available = 0;

                if(pseq->size_of_sequence == phand->hand_size){
                    save_sequence(pall_sequences, pseq);
                    pall_sequences->number_of_sequences++;
                }


                recursive_backtrack(phand, pseq, pall_sequences, inserted);

                block->available = 1;
                BLOCK *paux = pseq->pfirst->pprev;
                pseq->pfirst->pprev = pseq->pfirst->pprev->pprev;
                pseq->pfirst->pprev->pnext = NULL;

                free(paux);
                pseq->size_of_sequence--;
                inserted--;
            }
        }
        block = block->pnext;
    }
    return 0;
}

/**
 * is_current_assignment_consistent - Função responsável por verificar se a próxima peça a inserir na sequência é
 * consistente ao não. Se necessário a função is_current_assignment_consistent invoca funções auxiliares de forma
 * a inverter peças tornando a sequência consistente.
 * @param pseq - Apontador para a primeira peça da lista de sequência.
 * @param pnew_block - Apontador para a nova peça a inserir na sequencia.
 * @param inserted - Número de peças inseridas na sequencia.
 * @return (1/0) se a sequênia é ou não consistente com a nova peça.
 */
int is_current_assignment_consistent(SEQUENCE *pseq, BLOCK *pnew_block, short inserted) {
    if (inserted == 0) {
        return ASSIGNMENT_CONSISTENT;
    }else if (pnew_block->left_side == pseq->pfirst->pprev->right_side) {
        return ASSIGNMENT_CONSISTENT;
    }
    else if (pnew_block->right_side == pseq->pfirst->pprev->right_side) {
        invert_block(pnew_block);
        return ASSIGNMENT_CONSISTENT;
    }
    else if (inserted == 1 && pnew_block->left_side == pseq->pfirst->left_side ) {
        invert_block_sequence(pseq);
        return ASSIGNMENT_CONSISTENT;
    }
    else if (inserted == 1 && pnew_block->right_side == pseq->pfirst->left_side ) {
        invert_block_sequence(pseq);
        invert_block(pnew_block);
        return ASSIGNMENT_CONSISTENT;
    }
    else {
        return ASSIGNMENT_FAILURE;
    }
}

/**
 * invert_block - Inverte uma peça.
 * @param pblock - Peça a inverter.
 */
void invert_block(BLOCK *pblock) {
    short aux = 0;
    aux = pblock->left_side;
    pblock->left_side = pblock->right_side;
    pblock->right_side = aux;
}

/**
 * invert_block_sequence - Inverte a primeira peça da sequencia.
 * @param pseq - Apontador para a primeira peça da lista de sequência.
 */
void invert_block_sequence(SEQUENCE *pseq) {
    short aux = 0;
    aux = pseq->pfirst->left_side;
    pseq->pfirst->left_side = pseq->pfirst->right_side;
    pseq->pfirst->right_side = aux;
}

/**
 * save_sequence - Função responsável por guardar uma sequencia gerada na estrutura de dados que armazena todas
 * as sequencias (ALLSEQUENCES).
 * Antes de a guardar a função converte a lista de peças da sequência numa string e armazena-a
 * como string.
 * @param pall_sequences - Apontador para a lista de todas as sequências guardadas.
 * @param pSeq - Apontador para a sequência a guardar.
 */
void save_sequence(ALLSEQUENCES *pall_sequences, SEQUENCE *pSeq) {

    int i = 0, size_sequence = 0;

    SEQSTRING *newSequence = (SEQSTRING*)malloc(sizeof(SEQSTRING));
    newSequence->size_of_sequence = pSeq->size_of_sequence;
    newSequence->pnext = NULL;

    BLOCK *paux = pSeq->pfirst;

    char *sequence = (char *) malloc(sizeof(char) * (newSequence->size_of_sequence * 2) + 1);
    *sequence = '\0';

    for (i = 0; i < newSequence->size_of_sequence && paux != NULL; i++) {
        size_sequence = strlen(sequence);
        *(sequence + size_sequence)     = '0' + paux->left_side;
        *(sequence + (size_sequence+1)) = '0' + paux->right_side;
        *(sequence + (size_sequence+2)) = '\0';
        paux = paux->pnext;
    }

    newSequence->sequence = sequence;

    if(pall_sequences->pfirst == NULL){
        pall_sequences->pfirst = newSequence;
        //printf("%s", " pall_sequences->pfirst = newSequence;\n");
    }else{
        newSequence->pnext = pall_sequences->pfirst;
        pall_sequences->pfirst = newSequence;
    }

    if(pall_sequences->number_of_sequences%MEMORY_MAX_SEQUENCES == 0 && pall_sequences->number_of_sequences > 0)
    {
        save_sequences_file(pall_sequences);
        freeList(&pall_sequences->pfirst);
    }

}


void save_sequences_file(ALLSEQUENCES *pall_sequences){

    SEQSTRING *pauxT;
    pauxT = pall_sequences->pfirst;
    char buffer[150];


        while (pauxT!=NULL) {

            sprintf(buffer, "|%d*%s#\r\n", count,pauxT->sequence);


            if(strcmp(pauxT->sequence,"vazio")!=0)
            {
                //Escrever para um ficheiro
                write(destination,buffer,strlen(buffer));
            }
            strcpy(pauxT->sequence,"vazio");


            count++;
            pauxT = pauxT->pnext;

        }



}

void freeList(SEQSTRING **pfirst)
{
    SEQSTRING *tmp;
    while (*pfirst != NULL)
    {
        tmp = *pfirst;
        *pfirst = (*pfirst)->pnext;

        free(tmp);
    }
}

/**
 * print_sequences - Imprime todas as sequências guardadas.
 * @param allSequences - Apontador para a lista de sequencias.
 */
void print_sequences(ALLSEQUENCES *allSequences){

    SEQSTRING *pSeq = allSequences->pfirst;
        while (pSeq != NULL){
            printf("[%d]:%s\n",countPrint,pSeq->sequence);
            countPrint++;
            pSeq= pSeq->pnext;
        }
}
