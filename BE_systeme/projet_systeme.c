#include "projet_systeme.h"

//int erreur(int test, int code_erreur, char * error);
void *gestionnaire(void *);

//Fonction d'initialisations
int intiMsg(int nbre_abo, int taille_msg, int taille_boite){

    pthread_t id_gestionnaire;

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    if (flag_gestionnaire == 1) {
        perror("Erreur gestionnaire déjà lancé");
        return 1;

    }
    /*if(erreur(flag_gestionnaire, 1, "Gestionnaire deja lance") == 1)
        return 1;*/

    nombre_abonne = nbre_abo;
    tab_abonnes = (abonne *)malloc(nombre_abonne * sizeof(abonne));
    if ((int)(tab_abonnes) == -1){
        perror("Memoire non disponible nombre abonnes");
        return 1;
    }
    /*tab_abonnes = (abonne *)malloc(nbre_abo * sizeof(abonne));
    if (erreur((unsigned int)(tab_abonnes), 0, "Memoire non disponible nombre abonnes") == 1)
        return 1;*/

    if((cle_file_montante = ftok("gestionnaire", 8)) == -1){
        perror("Generation cle");
        return 1;
    }

    if((id_file_montante = msgget(cle_file_montante, 0600|IPC_CREAT)) == -1){
        perror("Ouverture de la file");
        return 1;
    }

    /*if (erreur(id_file_montante, -1, "Creation file montante") == 1)
        return 1;*/
    taille_max_boite = taille_boite;
    taille_message = taille_msg;

    if(pthread_create(&id_gestionnaire, NULL, gestionnaire, NULL) != 0){
        perror("Creation thread gestionnaire");
        return 1;
    }

    pthread_mutex_unlock(&_mutex);    //rend le mutex (initialisation finie)

    return 0;
}

//Fonction d'abonnement d'un thread
void aboMsg(pthread_t idThread){



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

/*int erreur(int test, int code_erreur, char * error){
    if (test == code_erreur){
        perror(error);
        return 1;
    }
    return 0;
}*/

void *gestionnaire(void * arg){

    message msgRecu;
    char* threadDest;
    int cle_dest, idDest;
    while(1){
    pthread_mutex_lock(&_mutex);
        while(flag_rcvMsg==0){
           // pthread_cond_wait(&_var_cond, &_mutex);
           msgRecu.destinataire = 0;
            if((msgrcv(id_file_montante,&msgRecu,sizeof(msgRecu),0,IPC_NOWAIT))==-1){
                perror("erreur de lecture dans la file montante\n");
                exit(EXIT_FAILURE);
            }
            if(msgRecu.destinataire!=0){
                //Casting de l'id du destinataire
                sprintf(threadDest,"%d",msgRecu.destinataire);

                //generation de la clé du destinataire
                if((cle_dest = ftok(threadDest, 8)) == -1){
                    perror("Generation cle");
                    exit(EXIT_FAILURE);
                }

                //Ouverture de la file du thread destinataire
                if((idDest = msgget(cle_dest, 0600|IPC_CREAT)) == -1){
                    perror("Ouverture de la file du thread dest");
                    exit(EXIT_FAILURE);
                }

                //Envoi du message dans la file du thread destinataire
                if(msgsnd(idDest, &msgRecu, sizeof(msgRecu),IPC_NOWAIT)==-1){
                    perror("Envoi de message dans la file du thread dest");
                    exit(EXIT_FAILURE);
                }

                //Incrémenter le compteur de message du thread destinataire
                int i=0, flag =0;
                while(flag == 0 && i<nombre_abonne){
                    if(tab_abonnes[i].id_abonne == msgRecu.destinataire){
                        tab_abonnes[i].nbre_messages++;
                        flag = 1;
                    }
                    i++;
                }
            }
        }
    pthread_mutex_unlock(&_mutex);
        sleep(1);
    }

}

