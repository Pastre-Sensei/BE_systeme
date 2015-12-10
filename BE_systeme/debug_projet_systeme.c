#include "projet_systeme.h"



//Fonction d'initialisations
int initMsg(int nbre_abo, int taille_msg, int taille_boite)
{
    #define DEBUG_INIT
    pthread_mutex_lock(&_mutex); //Prend le mutex

    if (flag_gestionnaire == 1)
    {
        pthread_mutex_unlock(&_mutex);
        #ifdef DEBUG_INIT
            perror("Gestionnaire deja lance");
        #endif
        return 3;
    }
    if(nbre_abo < 21)  //Regarde si l'utilisateur demande à faire communiquer plus ou moins de 20 threads
    {
        nombre_max_abonnes = nbre_abo;
    }
    else
        nombre_max_abonnes = 20; //On considère que l'utilisateur a demandé à faire communiquer trop de threads

    tab_abonnes = (abonne *)malloc(nombre_max_abonnes * sizeof(abonne));
    if ((int)(tab_abonnes) == -1)
    {
        #ifdef DEBUG_INIT
            perror("Memoire non disponible nombre abonnes");
        #endif
        return 4;
    }

    if((cle_file_montante = ftok("projet_systeme.c", 8)) == -1)
    {
        #ifdef DEBUG_INIT
            perror("Generation cle");
        #endif
        return 5;
    }

    #ifdef DEBUG_INIT
        printf("cle_file_montante : %d\n", cle_file_montante);
    #endif

    if((id_file_montante = msgget(cle_file_montante, 0600|IPC_CREAT)) == -1)
    {
        #ifdef DEBUG_INIT
            perror("Ouverture de la file");
        #endif
        return 5;
    }

    #ifdef DEBUG_INIT
        printf("id_file_montante : %d\n", id_file_montante);
    #endif

    if (taille_boite < 11)
        taille_max_boite = taille_boite;
    else
        taille_max_boite = 10;

    if (taille_msg < 81)
        taille_message = taille_msg;
    else
        taille_message = 80;

    if(pthread_create(&id_gestionnaire, NULL, gestionnaire, NULL) != 0)
    {
        #ifdef DEBUG_INIT
            perror("Creation thread gestionnaire");
        #endif
        return 5;
    }

    flag_gestionnaire = 1;
    pthread_mutex_unlock(&_mutex); //rend le mutex (initialisation finie)
    return 0;
}
//Fonction d'abonnement d'un thread
int aboMsg(pthread_t idThread)
{
    int i=0, flag =0, id_file, cle_thread;

#define DEBUG_ABO


#ifdef DEBUG_ABO
    printf("On appelle Abo !\n");
#endif

    pthread_mutex_lock(&_mutex); //Prend le mutex

#ifdef DEBUG_ABO
    printf("j arrive à prendre le mutex\n");
#endif

    if (test_gestionnaire() == 1)
    {
        pthread_mutex_unlock(&_mutex);
        return 1;
    }

//abonne ou pas
    while(flag == 0 && i<nombre_abonne)
    {
        if(tab_abonnes[i].id_abonne == idThread)
        {
            flag = 1;
        }
        i++;
    }

#ifdef DEBUG_ABO
    printf("flag : %d\n", flag);
#endif // DEBUG_ABO

    if(flag == 1)
    {
        #ifdef DEBUG_ABO
            perror("thread deja abonne\n");
        #endif
        return 6;
    }
//Nombre max d'abonnés atteint
    if (nombre_abonne == nombre_max_abonnes)
    {
        #ifdef DEBUG_ABO
            perror("Nombre max d'abonnés atteint\n");
        #endif
        return 4;
    }

#ifdef DEBUG_ABO
    printf("nbre : %d\n", nombre_abonne);
    printf("Generation cle du thread associe\n");
#endif // DEBUG_ABO

//generation de la clé du destinataire
    if((cle_thread = ftok("projet_systeme.c", idThread)) == -1)
    {
        #ifdef DEBUG_ABO
            perror("Generation cle du thread associe\n");
        #endif // DEBUG_ABO
        return 5;
    }

#ifdef DEBUG_ABO
    printf("cle : %d\n", cle_thread);
#endif // DEBUG_ABO

//Ouverture de la file du thread pour qu'il puisse recevoir des messages
    if((id_file = msgget(cle_thread, 0600|IPC_CREAT)) == -1)
    {
        #ifdef DEBUG_ABO
            perror("Ouverture de la file du thread\n");
        #endif // DEBUG_ABO
        return 5;
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
int desaboMsg(pthread_t idThread)
{
    int i=0, flag =0, posThread, messagesPerdus = 0, debug;
#define DEBUG_DESABO


#ifdef DEBUG_DESABO
    printf("Appel desabo %d\n", idThread);
#endif // DEBUG_DESABO

    pthread_mutex_lock(&_mutex); //Prend le mutex

    if (test_gestionnaire() == 1)
    {
        pthread_mutex_unlock(&_mutex);
        return 1;
    }

#ifdef DEBUG_DESABO
    printf("Je prend le mutex %d\n", flag_gestionnaire);
#endif // DEBUG_DESABO

//Cherche le thread dans la table des abonnes
    while(flag == 0 && i<nombre_abonne)
    {
        if(tab_abonnes[i].id_abonne == idThread)
        {
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
    if(flag == 0)
    {
        #ifdef DEBUG_DESABO
            perror("thread non abonne\n");
        #endif // DEBUG_DESABO
        return 2;
    }

#ifdef DEBUG_DESABO
    printf("Thread bien abonne\n");
#endif // DEBUG_DESABO

//Fermeture du flux lié au thread
    debug = msgctl(tab_abonnes[posThread].id_file_desc,IPC_RMID,NULL);
    if (debug != 0)
    {
        return 5;
    }
    for(i = posThread; i<nombre_abonne-1; i++)  //Compacter le tableau des abonnés
    {
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
int sendMsg(pthread_t dest, pthread_t exp, char *msgEnvoi)
{
    message messageSent; //la structure du message envoyée
    int i, flagDest, flagExp, posDest, ret_mutex_lock, ret_mutex_unlock;
    #define DEBUG_SEND

    ret_mutex_lock = pthread_mutex_lock(&_mutex); //Prend le mutex

    #ifdef DEBUG_SEND
        printf("On appelle sendMsg  en envoyant le message : %s\n", msgEnvoi);
    #endif // DEBUG_SEND

    if (test_gestionnaire() == 1)
    {
        pthread_mutex_unlock(&_mutex);
        return 1;
    }

//Verifier si le dest et l'exp sont abonnés
    i=0, flagDest =0, flagExp = 0;
    while((flagDest == 0 || flagExp == 0)&& i<nombre_abonne)
    {
        if(tab_abonnes[i].id_abonne == dest)  //le destinataire est un abonné ou pas
        {
            flagDest = 1;
            posDest = i;
        }
        if(tab_abonnes[i].id_abonne == exp)  //l'expéditeur est un abonné ou pas
        {
            flagExp = 1;
        }
        i++;
    }

    if(flagDest == 0 || flagExp == 0)
    {
        #ifdef DEBUG_SEND
            perror("un des deux threads n\'est pas abonne\n");
            #endif // DEBUG_SEND
        return 2;
    }

    #ifdef DEBUG_SEND
        printf("Tout le monde est bien abonne\n");
    #endif // DEBUG_SEND

//Verification si la boite du destinataire est pleine
    if(tab_abonnes[posDest].nbre_messages >= taille_max_boite)
    {
        #ifdef DEBUG_SEND
            perror("boite pleine du destinataire\n");
        #endif // DEBUG_SEND
        return 4;
    }

    #ifdef DEBUG_SEND
        printf("boite aux msgs du destinataire n\'est pas pleine\n");
    #endif // DEBUG_SEND

    messageSent.destinataire = dest;
    messageSent.expediteur = exp;
    char new_message[taille_message + 1];
    messageSent.msg= new_message;
    strncpy(messageSent.msg, msgEnvoi, taille_message); //Limite le message à la taille maximale définie lors de l'init"

    #ifdef DEBUG_SEND
        printf("j\'ai defini ma variable messageSent\n");
    #endif // DEBUG_SEND

    //Envoi du message dans la file du thread gestionnaire
    if(msgsnd(id_file_montante, &messageSent, sizeof(messageSent),IPC_NOWAIT)==-1)
    {
        #ifdef DEBUG_SEND
            perror("Envoi de message dans la file du thread gestionnaire");
        #endif // DEBUG_SEND
        return 5;
    }
//Incrémente la boite aux lettres du thread destinataire
    tab_abonnes[posDest].nbre_messages++;

    #ifdef DEBUG_SEND
        printf("***** C\'est OK ***** !\n");
    #endif // DEBUG_SEND

    pthread_cond_broadcast(&_var_cond);
    ret_mutex_unlock = pthread_mutex_unlock(&_mutex);

    #ifdef DEBUG_SEND
    printf("Je rend le mutex et val retour mutex unlock %d\n",ret_mutex_unlock);
    #endif // DEBUG_SEND

    return 0;
}
//fonction de recption de message
char* rcvMsg(pthread_t idThread, int nbre_msg_demande)
{

    int cle_thread, i=0, flag =0, posThread, id, ret_msgrcv =0;
    message message_recu;
        message_recu.destinataire=idThread;
        message_recu.expediteur=0;
        message_recu.msg = (char*)(malloc(taille_message*sizeof(char)));
    char nmbre_messages[2];
    char mes_messages[(nbre_msg_demande * taille_message) + nbre_msg_demande +1];
    #define DEBUG_RCV

    #ifdef DEBUG_RCV
        printf("On appelle rcvMsg\n");
    #endif // DEBUG_RCV

    pthread_mutex_lock(&_mutex); //Prend le mutex

    if (test_gestionnaire() == 1)   //Le gestionnaire est lancé ou pas
    {
        pthread_mutex_unlock(&_mutex);
        return "erreur gestionnaire non lance";
    }
//abonne ou pas
    while(flag == 0 && i<nombre_abonne)
    {
        if(tab_abonnes[i].id_abonne == idThread)
        {
            flag = 1;
            posThread = i;
        }
        i++;
    }

    if(flag==0)
    {
        #ifdef DEBUG_RCV
            perror("thread non abonne\n");
        #endif // DEBUG_RCV
        return "erreur : thread non abonne\n";
    }

//Test sur le nombre de messages à renvoyer
    if(nbre_msg_demande <= 0)  //là on renvoie juste le nombre de messages dispo pour le thread
    {
        sprintf(nmbre_messages,"%d", tab_abonnes[posThread].nbre_messages);
        return nmbre_messages;
    }


    //Generation de clé du thread courant
    if((cle_thread = ftok("projet_systeme.c", idThread)) == -1)
    {
        #ifdef DEBUG_RCV
            perror("Generation cle du thread associe\n");
        #endif // DEBUG_RCV
        return "erreur : communication";
    }

    //Ouverture ou creation de la file descendante du thread
    if((id = msgget(cle_thread, 0600|IPC_CREAT))==-1)
    {
        #ifdef DEBUG_RCV
            perror("creation ou ouverture echouee\n");
        #endif // DEBUG_RCV
        return "erreur : communication";
    }

    #ifdef DEBUG_RCV
        printf("ouverture du flux destinataire avec id : %d\n",id);
    #endif // DEBUG_RCV

    i = 0;
    while(i<nbre_msg_demande) //là on permet la recuperation des messages par le thread
    {
        //Recuperation des messages dans la boite aux lettres
        ret_msgrcv = msgrcv(id, &message_recu, sizeof(message_recu),0,IPC_NOWAIT);

        #ifdef DEBUG_RCV
            printf("recuperation des messages %s avec retour :  %d\n",message_recu.msg,ret_msgrcv);
        #endif // DEBUG_RCV

        if(ret_msgrcv == -1)
        {
            #ifdef DEBUG_RCV
                printf("pas de message dans le flux\n");
            #endif // DEBUG_RCV
            pthread_cond_wait(&_var_cond_rcv, &_mutex);
        }
        else{
            #ifdef DEBUG_RCV
                printf("message lu : %s\n",message_recu.msg);
            #endif // DEBUG_RCV
            strcat(mes_messages, message_recu.msg);
            strcat(mes_messages, "*");
            tab_abonnes[posThread].nbre_messages--;
            i++;
            free(message_recu.msg);
        }
    }
    pthread_mutex_unlock(&_mutex);
    return mes_messages;
}


int finMsg(int flag_fermeture)
{
    int ret, i ;
    pthread_mutex_lock(&_mutex);
    #define DEBUG_FIN


    if (test_gestionnaire() == 1)   //Ne peut pas arreter un gestionnaire non lancé
    {
        pthread_mutex_unlock(&_mutex);
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
        pthread_mutex_unlock(&_mutex);
        return ret; //retourner le nombre d'abonnés perdus
    }
    if(nombre_abonne!=0)
    {
        #ifdef DEBUG_FIN
            perror("il existe des abonnes\n");
        #endif // DEBUG_FIN
        ret = nombre_abonne;
        pthread_mutex_unlock(&_mutex);
        return ret;
    }
    //Le nombre des abonnés est nul donc on peut arreter le gestionnaire
    flag_gestionnaire = 0; //arret du gestionnaire
    msgctl(id_file_montante, IPC_RMID, NULL); //Suppression de la file montante du gestionnaire
    pthread_mutex_unlock(&_mutex);
    return 0;
}


void * gestionnaire(void *arg)
{
    message msgRecu;
    int idDest, i, flag, cle_dest, retour_msgrcv = 0;

    msgRecu.destinataire = 0;
    msgRecu.msg = (char*)(malloc(taille_message*sizeof(char)));
    #define DEBUG_GEST


    //while(flag_fermeture == 0)
    pthread_mutex_lock(&_mutex);

    #ifdef DEBUG_GEST
        printf("Le gestionnaire a le mutex\n");
    #endif // DEBUG_GEST

    while(flag_gestionnaire == 1)
    {
    //Récupère le message à transmettre (dans la file montante)
            retour_msgrcv = msgrcv(id_file_montante,&msgRecu,sizeof(msgRecu),0,IPC_NOWAIT);
            #ifdef DEBUG_GEST
            printf("******  test sur la recption de message : %d ***\n",retour_msgrcv);
            #endif // DEBUG_GEST

            if(retour_msgrcv==-1)
            {
                #ifdef DEBUG_GEST
                printf("Pas de message, le gestionnaire deverouille le mutex et attend que _var_cond soit signalee.\n");
                #endif // DEBUG_GEST
                pthread_cond_wait(&_var_cond, &_mutex);
                msgrcv(id_file_montante,&msgRecu,sizeof(msgRecu),0,IPC_NOWAIT);
            }

            #ifdef DEBUG_GEST
            printf("Le gestionnaire a recu le message : %s\n",msgRecu.msg);
            #endif // DEBUG_GEST

            //Incrémenter le compteur de message du thread destinataire et récupère l'id de sa file
            i=0, flag=0;
            while(flag == 0 && i<nombre_abonne)
            {
                if(tab_abonnes[i].id_abonne == msgRecu.destinataire)
                {
                    #ifdef DEBUG_GEST
                    printf("compteur msg  : %d\n",tab_abonnes[i].nbre_messages);
                    #endif // DEBUG_GEST
                    flag = 1;
//idDest = tab_abonnes[i].id_file_desc;
                }
                i++;
            }

            if(flag==1)//Le destinataire fait parti des abonnés
            {
                cle_dest = ftok("projet_systeme.c", msgRecu.destinataire);

//Ouverture de la file du thread destinataire (si non ouverte)
                if((idDest = msgget(cle_dest, 0600|IPC_CREAT)) == -1)
                {
                    #ifdef DEBUG_GEST
                    perror("Ouverture de la file du thread dest");
                    #endif // DEBUG_GEST
                }
                #ifdef DEBUG_GEST
                printf("flux du thread dest ouvert avec      avec idDest : %d\n",idDest);
                #endif // DEBUG_GEST

//Envoi du message dans la file du thread destinataire
                if(msgsnd(idDest, &msgRecu, sizeof(msgRecu),IPC_NOWAIT)==-1)
                {
                    #ifdef DEBUG_GEST
                    perror("Envoi de message dans la file du thread dest");
                    #endif // DEBUG_GEST
                }
                #ifdef DEBUG_GEST
                printf("envoi du msg dans le flux dest reussi\n");
                #endif // DEBUG_GEST

                pthread_cond_signal(&_var_cond_rcv);
            }

        sleep(1);
    }

    pthread_mutex_unlock(&_mutex);
    #ifdef DEBUG_GEST
    printf("le gestionnaire rend le mutex\n");
    #endif // DEBUG_GEST

    return 0;
}

int test_gestionnaire(void) //La fonction de test du lancement du gestionnaire, appelé souvent
{
    if (flag_gestionnaire == 0)
    {
        pthread_mutex_unlock(&_mutex);
        return 1;
    }
    return 0;
}

void * fonc_thread1 (void* arg) //La fonction qu'on donne au thread 1, utilisé pour les tests
{
    int abo_retour, desabo_retour, send_retour, fin_retour;
    pthread_t idThread;
    char rcv_retour[100];
    sleep(1);
    abo_retour = aboMsg(pthread_self());
    printf("Abo_retour : %d\n", abo_retour);
    idThread = pthread_self();
    printf("thread 1 :  %d\n", idThread);
     sleep(2);
    //send_retour = sendMsg(pthread_self(), pthread_self(), "Coucou");
    //sendMsg(pthread_self(), pthread_self(), "Djibrilla");
    //printf("Send retour = %d\n", send_retour);
    //sleep(2);
    //strncpy(rcv_retour, rcvMsg(pthread_self(), 2), 100);
    //printf("Thread 1 msg recu : %s\n", rcv_retour);

    //desabo_retour = desaboMsg(pthread_self());
    //printf("thread 1 desabo : %d\n", desabo_retour);

    fin_retour = finMsg(1);
    printf("thread 1 finMsg : %d\n", fin_retour);


    return 0;
}
void * fonc_thread2 (void* arg )    //La fonction qu'on donne au thread 2, utilisé pour les tests
{
    int abo_retour, desabo_retour;
    sleep(2);
    abo_retour = aboMsg(pthread_self());
    printf("thread 2 : %d\n", abo_retour);
    //sendMsg(tab_abonnes[0].id_abonne, pthread_self(), "Guillame");
    //printf("succes envoi msg du thread 2\n");
//    sleep(2);
    //desabo_retour = desaboMsg(pthread_self());
    //printf("thread 2 desabo : %d\n", desabo_retour);
    return 0;
}
int main()  //Un main de test, exemple qu'un utilisateur pourrait effectuer
{
    pthread_t thread1, thread2;
    int init_retour, fin_retour;
    init_retour = initMsg(3,50,50);
    printf("***main***\n");
    if(pthread_create(&thread1, NULL, fonc_thread1, NULL) != 0)
    {
        perror("Creation thread 1");
        return 1;
    }
    if(pthread_create(&thread2, NULL, fonc_thread2, NULL) != 0)
    {
        perror("Creation thread 2");
        return 1;
    }

    pthread_join(thread1, NULL);
    pthread_join(thread2, NULL);
    pthread_join(id_gestionnaire, NULL);
    fin_retour = finMsg(0);
    /*msgctl(tab_abonnes[0].id_file_desc,IPC_RMID,NULL);
    msgctl(tab_abonnes[1].id_file_desc,IPC_RMID,NULL);
    msgctl(id_file_montante, IPC_RMID,NULL);*/
    printf("Tout s'est bien passé\n");
    return 0;
}


