#include "projet_systeme.h"

//Fonction d'initialisations
int initMsg(int nbre_abo, int taille_msg, int taille_boite){


    //int retour_thread;

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    if (flag_gestionnaire == 1) {
        perror("Gestionnaire deja lance");
        return 1;
    }


    if(nbre_abo < 21){      //Regarde si l'utilisateur demande à faire communiquer plus ou moins de 20 threads
        nombre_max_abonnes = nbre_abo;
    }
    else
        nombre_max_abonnes = 20;    //On considère que l'utilisateur a demandé à faire communiquer trop de threads
    tab_abonnes = (abonne *)malloc(nombre_max_abonnes * sizeof(abonne));
    if ((int)(tab_abonnes) == -1){
        perror("Memoire non disponible nombre abonnes");
        return 1;
    }
    /*tab_abonnes = (abonne *)malloc(nbre_abo * sizeof(abonne));
    if (erreur((unsigned int)(tab_abonnes), 0, "Memoire non disponible nombre abonnes") == 1)
        return 1;*/

    if((cle_file_montante = ftok("projet_systeme.c", 8)) == -1){
        perror("Generation cle");
        return 1;
    }
    printf("%d\n", cle_file_montante);

    if((id_file_montante = msgget(cle_file_montante, 0600|IPC_CREAT)) == -1){
        perror("Ouverture de la file");
        return 1;
    }

    printf("%d\n", id_file_montante);

    if (taille_boite < 11){
        taille_max_boite = taille_boite;
    }
    else
        taille_max_boite = 10;

    if (taille_msg < 81)
        taille_message = taille_msg;
    else
        taille_message = 80;

    if(pthread_create(&id_gestionnaire, NULL, gestionnaire, NULL) != 0){
        perror("Creation thread gestionnaire");
        return 1;
    }

    flag_gestionnaire = 1;
    pthread_mutex_unlock(&_mutex);    //rend le mutex (initialisation finie)


    return 0;
}
