#include "projet_systeme.h"

void * gestionnaire(void * arg){

    message msgRecu;
    char* threadDest;
    int idDest, i=0, flag=0, cle_dest;
    //while(flag_fermeture == 0)
    while(flag_gestionnaire == 1){
    pthread_mutex_lock(&_mutex);
        while(flag_rcvMsg==0){
        sleep(1);
           // pthread_cond_wait(&_var_cond, &_mutex);

           //Récupère le message à transmettre (dans la file montante)
           msgRecu.destinataire = 0;
            if((msgrcv(id_file_montante,&msgRecu,sizeof(msgRecu),0,IPC_NOWAIT))==-1){
                //perror("erreur de lecture dans la file montante\n");
                pthread_cond_wait(&_var_cond, &_mutex);
            }
            if(msgRecu.destinataire!=0){

                //Incrémenter le compteur de message du thread destinataire et récupère l'id de sa file
                while(flag == 0 && i<nombre_abonne){
                    if(tab_abonnes[i].id_abonne == msgRecu.destinataire){
                        tab_abonnes[i].nbre_messages++;
                        flag = 1;
                        //idDest = tab_abonnes[i].id_file_desc;
                    }
                    i++;
                }

                //sprintf(threadDest, "%d", msgRecu.destinataire);
                cle_dest = ftok("projet_systeme.c", msgRecu.destinataire);

                //Ouverture de la file du thread destinataire (si non ouverte)
                if((idDest = msgget(cle_dest, 0600|IPC_CREAT)) == -1){
                    perror("Ouverture de la file du thread dest");
                }

                //Envoi du message dans la file du thread destinataire
                if(msgsnd(idDest, &msgRecu, sizeof(msgRecu),IPC_NOWAIT)==-1){
                    perror("Envoi de message dans la file du thread dest");
                }
            }
            sleep(1);
        }

    pthread_mutex_unlock(&_mutex);
    sleep(1);
    }
    return 0;
}

int test_gestionnaire(void){

    if (flag_gestionnaire == 0) {
        pthread_mutex_unlock(&_mutex);
        perror("Gestionnaire non lance\n");
        return 1;
    }
    return 0;

}

