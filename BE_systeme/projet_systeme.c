#include "projet_systeme.h"



//Fonction d'initialisations
int initMsg(int nbre_abo, int taille_msg, int taille_boite){

    #define DEBUG_INIT
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


    if((cle_file_montante = ftok("projet_systeme.c", 8)) == -1){
        perror("Generation cle");
        return 1;
    }
    printf("cle_file_montante : %d\n", cle_file_montante);

    if((id_file_montante = msgget(cle_file_montante, 0600|IPC_CREAT)) == -1){
        perror("Ouverture de la file");
        return 1;
    }

    printf("id_file_montante : %d\n", id_file_montante);

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

//Fonction d'abonnement d'un thread
int aboMsg(pthread_t idThread){
    int i=0, flag =0, id_file, cle_thread;

    //#define DEBUG_ABO

    #ifdef DEBUG_ABO
    printf("On appelle abo !\n");
    #endif


    pthread_mutex_lock(&_mutex);    //Prend le mutex
    #ifdef DEBUG_ABO
    printf("abo : j arrive à prendre le mutex\n");
    #endif // DEBUG_ABO
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
    #ifdef DEBUG_ABO
    printf("flag : %d\n", flag);
    #endif // DEBUG_ABO

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
    #ifdef DEBUG_ABO
    printf("nbre : %d\n", nombre_abonne);
    #endif // DEBUG_ABO



    //generation de la clé du destinataire
    #ifdef DEBUG_ABO
    printf("Generation cle du thread associe\n");
    #endif // DEBUG_ABO

    if((cle_thread = ftok("projet_systeme.c", idThread)) == -1)
    {
        perror("Generation cle du thread associe\n");
        return 1;
    }
    #ifdef DEBUG_ABO
    printf("cle : %d\n", cle_thread);
    #endif // DEBUG_ABO


    //Ouverture de la file du thread pour qu'il puisse recevoir des messages
    if((id_file = msgget(cle_thread, 0600|IPC_CREAT)) == -1)
    {
        perror("Ouverture de la file du thread\n");
        return 1;
    }
    //Insérer le thread dans la table des abonnés
    tab_abonnes[nombre_abonne].id_abonne = idThread;
    tab_abonnes[nombre_abonne].nbre_messages = 0;
    tab_abonnes[nombre_abonne].id_file_desc = id_file;

    //Incrémenter le nombre des abonnés
    nombre_abonne++;

    pthread_mutex_unlock(&_mutex);
    #ifdef DEBUG_ABO
    printf("je rend le mutex\n");
    #endif // DEBUG_ABO

    return 0;
}

//fonction de desabonnement d'un thread utilisateur
int desaboMsg(pthread_t idThread){

    int i=0, flag =0, posThread, messagesPerdus = 0, debug;

    //#define DEBUG_DESABO

    #ifdef DEBUG_DESABO
    printf("Appel desabo %d\n", idThread);
    #endif // DEBUG_DESABO

    pthread_mutex_lock(&_mutex);    //Prend le mutex
    if (test_gestionnaire() == 1) {
        return 1;
    }
    #ifdef DEBUG_DESABO
    printf("Je prend le mutex %d\n", flag_gestionnaire);
    #endif // DEBUG_DESABO


    while(flag == 0 && i<nombre_abonne){
        if(tab_abonnes[i].id_abonne == idThread){
            flag = 1;
            posThread = i;
            messagesPerdus = tab_abonnes[i].nbre_messages;
        }
        i++;
    }
    #ifdef DEBUG_DESABO
    printf("La boucle est passee\n");
    #endif // DEBUG_DESABO

    //Le thread ne s'est pas abonné
    if(flag == 0){
        perror("thread non abonne\n");
        return 1;
    }
    #ifdef DEBUG_DESABO
    printf("Thread bien abonne\n");
    #endif // DEBUG_DESABO

    //Fermeture du flux lié au thread
    debug = msgctl(tab_abonnes[posThread].id_file_desc,IPC_RMID,NULL);
    printf("retour msgctl de desabo : %d\n",debug);

    for(i = posThread; i<nombre_abonne-1; i++){ //Compacter le tableau des abonnés
        tab_abonnes[i] = tab_abonnes[i+1];
    }
    #ifdef DEBUG_DESABO
    printf("id : %d\n", tab_abonnes[0].id_abonne);
    #endif // DEBUG_DESABO

    //Décrementer le nombre d'abonnés dans la table
    nombre_abonne--;

    #ifdef DEBUG_DESABO
    printf("Nombre abonnes restant : %d\n", nombre_abonne);
    #endif // DEBUG_DESABO


    pthread_mutex_unlock(&_mutex);
    #ifdef DEBUG_DESABO
    printf("Mutex rendu, msgctl : %d messagesPerdus %d\n", debug, messagesPerdus);
    #endif // DEBUG_DESABO

    return messagesPerdus;
}

//fonction d'envoi de message
int sendMsg(pthread_t dest, pthread_t exp, char *msgEnvoi){

    message messageSent; //la structure du message envoyée
    int posDest;
    printf("On appelle sendMsg   %s\n", msgEnvoi);

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

    messageSent.msg = (char*)(malloc(taille_message * sizeof(char)));

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

//fonction de recption de message
int rcvMsg(pthread_t idThread, int nbre_msg_demande){

    int cle_thread, i=0, flag =0, posThread, id;
    message message_recu;

    pthread_mutex_lock(&_mutex);    //Prend le mutex

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

   // sprintf(threadDest,"%d",idThread);
    cle_thread = ftok("projet_systeme.c",idThread); //Generation de clé du thread courant

    //Ouverture ou creation de la file descendante du thread
    if((id = msgget(cle_thread, 0600|IPC_CREAT))==-1){
        perror("creation ou ouverture echouee\n");
        return 1;
    }

    i = 0;
    while(i<nbre_msg_demande) //là on permet la recuperation des messages par le thread
    {
        //Recuperation des messages dans la boite aux lettres
        if(msgrcv(id, &message_recu, sizeof(message_recu),0,0) == 0)
        {
            printf("message lu : %s\n",message_recu.msg);
            tab_abonnes[posThread].nbre_messages--;
            i++;
            free(&message_recu.msg);
        }
    }

    pthread_mutex_unlock(&_mutex);
    return 0;
}

int finMsg(int flag_fermeture)
{
    int ret, i ;
    pthread_mutex_lock(&_mutex);
    if (test_gestionnaire() == 1)   //Ne peut pas arreter un gestionnaire non lancé
    {
        return 1;
    }

    if (flag_fermeture == 1)
    {
        //fermeture de tous les threads abonnés
        for( i = 0; i<nombre_abonne; i++)
        {
            msgctl(tab_abonnes[i].id_file_desc,IPC_RMID,NULL);
        }

        ret = nombre_abonne;
        nombre_abonne = 0; //Vider le tableau des abonnés
        flag_gestionnaire = 0; //arret du gestionnaire
        msgctl(id_file_montante, IPC_RMID, NULL); //Suppression de la file montante du gestionnaire
        return ret; //retourner le nombre d'abonnés perdus
    }

    if(nombre_abonne!=0)
    {
        perror("il existe des abonnes\n");
        ret = nombre_abonne;
        return ret;
    }

    flag_gestionnaire = 0; //arret du gestionnaire
    msgctl(id_file_montante, IPC_RMID, NULL); //Suppression de la file montante du gestionnaire

    pthread_mutex_unlock(&_mutex);

    return 0;
}

void * gestionnaire(void * arg)
{

    message msgRecu;
    int idDest, i, flag, cle_dest;
    msgRecu.msg = (char*)(malloc(taille_message * sizeof(char)));
#define DEBUG_GEST
    //while(flag_fermeture == 0)
    while(flag_gestionnaire == 1)
    {
        pthread_mutex_lock(&_mutex);
#ifdef DEBUG_GEST
        printf("Le gestionnaire a le mutex\n");
#endif // DEBUG_GEST
        sleep(1);
        // pthread_cond_wait(&_var_cond, &_mutex);

        //Récupère le message à transmettre (dans la file montante)
        while((msgrcv(id_file_montante,&msgRecu,sizeof(msgRecu),0,IPC_NOWAIT))==0)
        {
            //perror("erreur de lecture dans la file montante\n");
        #ifdef DEBUG_GEST
            printf("Le gestionnaire a recu un message %d  %s\n",msgRecu.destinataire, msgRecu.msg);
        #endif // DEBUG_GEST

            //Incrémenter le compteur de message du thread destinataire et récupère l'id de sa file
            i=0, flag=0;
            while(flag == 0 && i<nombre_abonne)
            {
                if(tab_abonnes[i].id_abonne == msgRecu.destinataire)
                {
                    tab_abonnes[i].nbre_messages++;
                    printf("nombre de msgs dispo : %d\n",tab_abonnes[i].nbre_messages);
                    flag = 1;
                    //idDest = tab_abonnes[i].id_file_desc;
                }
                i++;
            }

        #ifdef DEBUG_GEST
            printf("Tableau actualise\n");
        #endif // DEBUG_GEST

            //sprintf(threadDest, "%d", msgRecu.destinataire);
            cle_dest = ftok("projet_systeme.c", msgRecu.destinataire);

            //Ouverture de la file du thread destinataire (si non ouverte)
            if((idDest = msgget(cle_dest, 0600|IPC_CREAT)) == -1)
            {
                perror("Ouverture de la file du thread dest");
            }

        #ifdef DEBUG_GEST
            printf("File thread ouverte\n");
        #endif // DEBUG_GEST

            //Envoi du message dans la file du thread destinataire
            if(msgsnd(idDest, &msgRecu, sizeof(msgRecu),IPC_NOWAIT)==-1)
            {
                perror("Envoi de message dans la file du thread dest");
            }
        #ifdef DEBUG_GEST
            printf("Message envoye\n");
        #endif // DEBUG_GEST

        }
        #ifdef DEBUG_GEST
        printf("Pas de message, le gestionnaire laisse le mutex\n");
        #endif // DEBUG_GEST
        pthread_cond_wait(&_var_cond, &_mutex);

        //sleep(1);

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

void * fonc_thread1 (void * arg){

    int abo_retour, desabo_retour, send_retour;
    pthread_t idthread;

    sleep(1);
    idthread = pthread_self();
    abo_retour = aboMsg(idthread);
    printf("abo thread %d : %d\n",idthread ,abo_retour);
   // sleep(2);
    send_retour = sendMsg(idthread, idthread, "Coucou");
    printf("Send retour = %d\n", send_retour);
    //sleep(2);
    //desabo_retour = desaboMsg(pthread_self());
    printf("tableau %d \n", tab_abonnes[0].id_abonne);

//    printf("thread %d : %d\n", desabo_retour);

    return 0;
}

void * fonc_thread2 (void * arg){

    int abo_retour, desabo_retour;
//    sleep(2);
//    abo_retour = aboMsg(pthread_self());
//    printf("thread 2 : %d\n", abo_retour);
//    sleep(2);
//    desabo_retour = desaboMsg(pthread_self());
//    printf("thread 2 : %d\n", desabo_retour);

    return 0;
}



int main(){

    pthread_t thread1, thread2;

    initMsg(3,50,50);
    printf("***main***\n");

    if(pthread_create(&thread1, NULL, fonc_thread1, NULL) != 0){
        perror("Creation thread 1");
        return 1;
    }

    if(pthread_create(&thread2, NULL, fonc_thread2, NULL) != 0){
        perror("Creation thread 2");
        return 1;
    }
    //sleep(3);
  //  sendMsg(tab_abonnes[1].id_abonne, tab_abonnes[0].id_abonne, "Coucou");
   // printf("Reussi du premier coup\n");


    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(id_gestionnaire, NULL);





    msgctl(tab_abonnes[0].id_file_desc,IPC_RMID,NULL);
    msgctl(tab_abonnes[1].id_file_desc,IPC_RMID,NULL);
    msgctl(id_file_montante, IPC_RMID,NULL);
    printf("Tout c'est bien passé\n");
    return 0;
}



