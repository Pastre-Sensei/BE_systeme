#define _REENTRANT
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/msg.h>

typedef struct {
    pthread_t id_abonne;
    unsigned int nbre_messages;
} abonne;

typedef struct {
    pthread_t destinataire;
    pthread_t expediteur;
    char * msg;
} message;

abonne *tab_abonnes;
int nombre_abonne;
int taille_message = 100, taille_max_boite;
int cle_file_montante, id_file_montante;
int flag_gestionnaire = 0;
int flag_rcvMsg = 0;
pthread_cond_t _var_cond = PTHREAD_COND_INITIALIZER;
pthread_mutex_t _mutex = PTHREAD_MUTEX_INITIALIZER;


