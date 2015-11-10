#include "projet_systeme.h"

//int erreur(int test, int code_erreur, char * error);
void *gestionnaire(void *);
int test_gestionnaire(void);

//Fonction d'initialisations
int initMsg(int nbre_abo, int taille_msg, int taille_boite){

    pthread_t id_gestionnaire;

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    if (flag_gestionnaire == 1) {
        perror("Gestionnaire deja lance");
        return 1;
    }

    nombre_max_abonnes = nbre_abo;
    tab_abonnes = (abonne *)malloc(nombre_max_abonnes * sizeof(abonne));
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

    flag_gestionnaire = 1;
    pthread_mutex_unlock(&_mutex);    //rend le mutex (initialisation finie)

    return 0;
}

//Fonction d'abonnement d'un thread
int aboMsg(pthread_t idThread){

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    if (test_gestionnaire() == 1) {
        return 1;
    }

    //abonne ou pas
    int i=0, flag =0;
    while(flag == 0 && i<nombre_abonne){
        if(tab_abonnes[i].id_abonne == idThread){
            flag = 1;
        }
        i++;
    }
    if(flag == 1){
        perror("thread deja abonne\n");
        return 1;
    }


    //Nombre max d'abonnés atteint
    if (nombre_abonne == nombre_max_abonnes){
        perror("Nombre max d'abonnés atteint\n");
        return 1;
    }

    //Insérer le thread dans la table des abonnés
    tab_abonnes[nombre_abonne].id_abonne = idThread;
    tab_abonnes[nombre_abonne].nbre_messages++;

    //Incrémenter le nombre des abonnés
    nombre_abonne++;

    pthread_mutex_unlock(&_mutex);

    return 0;
}

//fonction de desabonnement d'un thread utilisateur
int desaboMsg(pthread_t idThread){

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    if (test_gestionnaire() == 1) {
        return 1;
    }

    int i=0, flag =0; int posThread, messagesPerdus;
    while(flag == 0 && i<nombre_abonne){
        if(tab_abonnes[i].id_abonne == idThread){
            flag = 1;
            posThread = i;
            messagesPerdus = tab_abonnes[i].nbre_messages;
        }
        i++;
    }

    //Le thread ne s'est pas abonné
    if(flag == 0){
        perror("thread non abonne\n");
        return 1;
    }

    for(int i = posThread; i<nombre_abonne-1; i++){
        tab_abonnes[i] = tab_abonnes[i+1];
    }
    //Décrementer le nombre d'abonnés dans la table
    nombre_abonne--;

    pthread_mutex_unlock(&_mutex);
    return messagesPerdus;
}

//fonction d'envoi de message
int sendMsg(pthread_t dest, pthread_t exp, char *msgEnvoi){

    message messageSent; //la structure du message envoyée
    int posDest;

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    if (flag_gestionnaire == 0) {
        pthread_mutex_unlock(&_mutex);
        perror("Gestionnaire non lance\n");
        return 1;
    }

    //Verifier si le dest et l'exp sont abonnés
    int i=0, flagDest =0, flagExp = 0;
    while((flagDest == 0 || flagExp == 0)&& i<nombre_abonne){
        if(tab_abonnes[i].id_abonne == dest){ //le destinataire est un abonné ou pas
            flagDest = 1;
            posDest = i;
        }

        if(tab_abonnes[i].id_abonne == exp){ //l'expéditeur est un abonné ou pas
            flagExp = 1;
        }
        i++;
    }

    if(flagDest == 0 || flagExp == 0){
        perror("un des deux threads n\'est pas abonne\n");
        return 1;
    }

    //Verification si la boite du destinataire est pleine
    if(tab_abonnes[posDest].nbre_messages == taille_max_boite){
        perror("boite pleine du destinataire\n");
        return 1;
    }

    messageSent.destinataire = dest;
    messageSent.expediteur = exp;
    strncpy(messageSent.msg, taille_message, msgEnvoi);



    tab_abonnes[posDest].nbre_messages++;
    pthread_mutex_unlock(&mutex);
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


int test_gestionnaire(void){

    if (flag_gestionnaire == 0) {
        pthread_mutex_unlock(&_mutex);
        perror("Gestionnaire non lance\n");
        return 1;
    }
    return 0;

}




