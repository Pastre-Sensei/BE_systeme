#include "projet_systeme.h"
extern nombre_abonne;
extern
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
