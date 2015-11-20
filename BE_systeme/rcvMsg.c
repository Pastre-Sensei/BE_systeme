#include "projet_systeme.h"


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
