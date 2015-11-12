#include "projet_systeme.h"

//int erreur(int test, int code_erreur, char * error);
void * gestionnaire(void *);
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
    int i=0, flag =0, id_file, cle_thread;
    char *chaineThread;


    pthread_mutex_lock(&_mutex);    //Prend le mutex
    if (test_gestionnaire() == 1) {
        return 1;
    }

    //abonne ou pas
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
    if (nombre_abonne == nombre_max_abonnes)
    {
        perror("Nombre max d'abonnés atteint\n");
        return 1;
    }

    //Casting de l'id du destinataire
    sprintf(chaineThread,"%d",idThread);

    //generation de la clé du destinataire
    if((cle_thread = ftok(chaineThread, 8)) == -1)
    {
        perror("Generation cle du thread associe\n");
        return 1;
    }

    //Ouverture de la file du thread destinataire
    if((id_file = msgget(cle_thread, 0600|IPC_CREAT)) == -1)
    {
        perror("Ouverture de la file du thread\n");
        return 1;
    }
    //Insérer le thread dans la table des abonnés
    tab_abonnes[nombre_abonne].id_abonne = idThread;
    tab_abonnes[nombre_abonne].nbre_messages++;
    tab_abonnes[nombre_abonne].id_file_desc = id_file;

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


    //Fermeture du flux lié au thread

    msgctl(tab_abonnes[posThread].id_file_desc,IPC_RMID,NULL);

    for(i = posThread; i<nombre_abonne-1; i++){ //Compacter le tableau des abonnés
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
    if (test_gestionnaire() == 1) {
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

    //Envoi du message dans la file du thread gestionnaire
    if(msgsnd(id_file_montante, &messageSent, sizeof(messageSent),IPC_NOWAIT)==-1){
        perror("Envoi de message dans la file du thread gestionnaire");
        return 1;
    }


    //Incrémente la boite aux lettres du thread destinataire
    tab_abonnes[posDest].nbre_messages++;
    pthread_mutex_unlock(&_mutex);

    return 0;
}

//fonction de recption de message
int rcvMsg(pthread_t idThread, int nbre_msg_demande){

    char* threadDest;
    int cle_thread, i=0, flag =0, posThread, id;
    message message_recu;

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    flag_rcvMsg = 1; //reprendre la main au gestionnaire

    if (test_gestionnaire() == 1) { //Le gestionnaire est lancé ou pas
        return 1;
    }

     //abonne ou pas
    while(flag == 0 && i<nombre_abonne){
        if(tab_abonnes[i].id_abonne == idThread){
            flag = 1;
            posThread = i;
        }
        i++;
    }

    if(flag==0){
        perror("thread non abonne\n");
        return 1;
    }

    //Test sur le nombre de messages à renvoyer
    if(nbre_msg_demande <= 0){ //là on renvoie juste le nombre de messages dispo pour le thread
        return tab_abonnes[posThread].nbre_messages;
    }

    sprintf(threadDest,"%d",idThread);
    cle_thread = ftok(threadDest,8); //Generation de clé du thread courant

    //Ouverture ou creation de la file descendante du thread
    if((id = msgget(cle_thread, 0600|IPC_CREAT))==-1){
        perror("creation ou ouverture echouee\n");
        return 1;
    }

    i = 0;
    while(i<nbre_msg_demande){//là on permet la recuperation des messages par le thread
        //Recuperation des messages dans la boite aux lettres
        if(msgrcv(id, &message_recu, sizeof(message_recu),0,0) == 0){
            printf("message lu : %s\n",message_recu.msg);
            tab_abonnes[posThread].nbre_messages--;
            i++;
        }
        flag_rcvMsg = 0;
    }

    flag_rcvMsg = 0;

    pthread_mutex_unlock(&_mutex);
    return 0;
}


int finMsg(int flag_fermeture){
    int ret, i ;
    pthread_mutex_lock(&_mutex);
    if (test_gestionnaire() == 1) { //Le gestionnaire est lancé ou pas
        return 1;
    }

    if (flag_fermeture == 1){
        //fermeture de tous les threads abonnés
        for( i = 0; i<nombre_abonne; i++){
            msgctl(tab_abonnes[i].id_file_desc,IPC_RMID,NULL);
        }

        ret = nombre_abonne;
        nombre_abonne = 0; //Vider le tableau des abonnés
        flag_gestionnaire = 0; //arret du gestionnaire
        msgctl(id_file_montante, IPC_RMID, NULL); //Suppression de la file montante du gestionnaire
        return ret; //retourner le nombre d'abonnés perdus
    }

    if(nombre_abonne!=0){
        perror("il existe des abonnes\n");
        ret = nombre_abonne;
        return ret;
    }

    flag_gestionnaire = 0; //arret du gestionnaire
    msgctl(id_file_montante, IPC_RMID, NULL); //Suppression de la file montante du gestionnaire

    pthread_mutex_unlock(&_mutex);

    return 0;
}

/*int erreur(int test, int code_erreur, char * error){
    if (test == code_erreur){
        perror(error);
        return 1;
    }
    return 0;
}*/

void * gestionnaire(void * arg){

    message msgRecu;
    char* threadDest;
    int cle_dest, idDest;
    //while(flag_fermeture == 0)
    while(flag_gestionnaire == 1){
    pthread_mutex_lock(&_mutex);
        while(flag_rcvMsg==0){
           // pthread_cond_wait(&_var_cond, &_mutex);
           msgRecu.destinataire = 0;
            if((msgrcv(id_file_montante,&msgRecu,sizeof(msgRecu),0,IPC_NOWAIT))==-1){
                perror("erreur de lecture dans la file montante\n");
            }
            if(msgRecu.destinataire!=0){
                //Casting de l'id du destinataire
                sprintf(threadDest,"%d",msgRecu.destinataire);

                //generation de la clé du destinataire
                if((cle_dest = ftok(threadDest, 8)) == -1){
                    perror("Generation cle");
                }

                //Ouverture de la file du thread destinataire
                if((idDest = msgget(cle_dest, 0600|IPC_CREAT)) == -1){
                    perror("Ouverture de la file du thread dest");
                }

                //Envoi du message dans la file du thread destinataire
                if(msgsnd(idDest, &msgRecu, sizeof(msgRecu),IPC_NOWAIT)==-1){
                    perror("Envoi de message dans la file du thread dest");
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
    exit(0);
}


int test_gestionnaire(void){

    if (flag_gestionnaire == 0) {
        pthread_mutex_unlock(&_mutex);
        perror("Gestionnaire non lance\n");
        return 1;
    }
    return 0;

}




