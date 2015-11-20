#include "projet_systeme.h"


//fonction d'envoi de message
int sendMsg(pthread_t dest, pthread_t exp, char *msgEnvoi){

    message messageSent; //la structure du message envoyée
    int posDest;
    printf("On appelle sendMsg %s\n", msgEnvoi);

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    printf("J arrive a prendre le mutex\n");
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
    printf("Tout le monde est bien abonne\n");


    //Verification si la boite du destinataire est pleine
    if(tab_abonnes[posDest].nbre_messages == taille_max_boite){
        perror("boite pleine du destinataire\n");
        return 1;
    }

    messageSent.destinataire = dest;
    messageSent.expediteur = exp;
    strncpy(messageSent.msg, msgEnvoi, taille_message); //Limite le message à la taille maximale définie lors de l'init"

    //Envoi du message dans la file du thread gestionnaire
    if(msgsnd(id_file_montante, &messageSent, sizeof(messageSent),IPC_NOWAIT)==-1){
        perror("Envoi de message dans la file du thread gestionnaire");
        return 1;
    }
    printf("C est parti !\n");


    //Incrémente la boite aux lettres du thread destinataire
    tab_abonnes[posDest].nbre_messages++;
    pthread_cond_signal(&_var_cond);
    pthread_mutex_unlock(&_mutex);
    printf("Je rend le mutex\n");

    return 0;
}
