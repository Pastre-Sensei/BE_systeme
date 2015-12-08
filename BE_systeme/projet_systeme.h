#ifndef _PROJET_SYSTEME_H
#define _PROJET_SYSTEME_H
#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>
/* gcc -Wall -pthread -o projet_systeme projet_systeme.c*/
typedef struct
{
    pthread_t id_abonne;
    int nbre_messages;
    int id_file_desc;
} abonne;
typedef struct
{
    pthread_t destinataire;
    pthread_t expediteur;
    char * msg;
} message;
abonne *tab_abonnes;
int nombre_max_abonnes;
int nombre_abonne = 0;
int taille_message = 100, taille_max_boite;
int cle_file_montante, id_file_montante;
int flag_gestionnaire = 0;
int flag_rcvMsg = 0;
pthread_t id_gestionnaire;
pthread_cond_t _var_cond = PTHREAD_COND_INITIALIZER;
pthread_cond_t _var_cond_rcv = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;
//Déclarations des fonctions
int initMsg(int a, int b, int c);
int aboMsg(pthread_t a);
int desaboMsg(pthread_t a);
int sendMsg(pthread_t dest, pthread_t exp, char * msg);
char* rcvMsg(pthread_t id, int nbre_msg);
int finMsg(int flag);
void * gestionnaire(void * arg);
int test_gestionnaire(void);
#endif // includes

