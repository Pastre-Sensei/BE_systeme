#include "projet_systeme.h"


//Fonction d'abonnement d'un thread
int aboMsg(pthread_t idThread){
    int i=0, flag =0, id_file, cle_thread;
    char *chaineThread;

    printf("On m'appelle !\n");
    pthread_mutex_lock(&_mutex);    //Prend le mutex
    printf("j arrive à prendre le mutex\n");
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
    printf("flag : %d\n", flag);
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
    printf("nbre : %d\n", nombre_abonne);


    //generation de la clé du destinataire
    printf("Generation cle du thread associe\n");
    if((cle_thread = ftok("projet_systeme.c", idThread)) == -1)
    {
        perror("Generation cle du thread associe\n");
        return 1;
    }
    printf("cle : %d\n", cle_thread);

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
    printf("je rend le mutex\n");

    return 0;
}
