#include "projet_systeme.h"

int finMsg(int flag_fermeture){
    int ret, i ;
    pthread_mutex_lock(&_mutex);
    if (test_gestionnaire() == 1) { //Ne peut pas arreter un gestionnaire non lancé
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
