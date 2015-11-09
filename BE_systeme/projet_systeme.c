#include "projet_systeme.h"

int erreur(int test, int code_erreur, char * error);
void gestionnaire(void *);

//Fonction d'initialisations
int intiMsg(int nbre_abo, int taille_msg, int taille_boite){

    pthread_t id_gestionnaire;

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    /*if (flag_gestionnaire == 1) {
        perror("Erreur gestionnaire déjà lancé");
        return 1;

    }*/
    if(erreur(flag_gestionnaire, 1, "Gestionnaire deja lance") == 1)
        return 1;

    pthread_mutex_unlock(&_mutex);      //rend le mutex (test fini)
    /*if ((tab_abonnes = (abonne *)malloc(nbre_abo * sizeof(abonne))) == -1){
        perror("Memoire non disponible nombre abonnes");
        return 1;
    }*/
    tab_abonnes = (abonne *)malloc(nbre_abo * sizeof(abonne));
    if (erreur((unsigned int)(tab_abonnes), 0, "Memoire non disponible nombre abonnes") == 1)
        return 1;
    cle_file_montante = ftok("gestionnaire", 8);
    id_file_montante = msgget(cle_file_montante, 0600|IPC_CREAT);
    if (erreur(id_file_montante, -1, "Creation file montante") == 1)
        return 1;
    taille_max_boite = taille_boite;
    taille_message = taille_msg;

    if(pthread_create(&id_gestionnaire, NULL, gestionnaire, NULL) != 0){
        perror("Creation thread gestionnaire");
        return 1;
    }


    return 0;
}

//Fonction d'abonnement d'un thread
void aboMsg(){


}

//fonction de desabonnement d'un thread utilisateur
void desaboMsg(){


}

//fonction d'envoi de message
void sendMsg(){

}

//fonction de recption de message
void rcvMsg(){


}

int erreur(int test, int code_erreur, char * error){
    if (test == code_erreur){
        perror(error);
        return 1;
    }
    return 0;
}

void gestionnaire(void * arg){

}

