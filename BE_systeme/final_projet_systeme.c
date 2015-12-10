#include "projet_systeme.h"


//Fonction d'initialisations
int initMsg(int nbre_abo, int taille_msg, int taille_boite)
{
    pthread_mutex_lock(&_mutex); //Prend le mutex

    if (flag_gestionnaire == 1)
    {
        pthread_mutex_unlock(&_mutex);
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
        return 4;
    }

    if((cle_file_montante = ftok("projet_systeme.c", 8)) == -1)
    {
        return 5;
    }

    if((id_file_montante = msgget(cle_file_montante, 0600|IPC_CREAT)) == -1)
    {
        return 5;
    }

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

    pthread_mutex_lock(&_mutex); //Prend le mutex

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

    if(flag == 1)
    {
        return 6;
    }
//Nombre max d'abonnés atteint
    if (nombre_abonne == nombre_max_abonnes)
    {
        return 4;
    }

//generation de la clé du destinataire
    if((cle_thread = ftok("projet_systeme.c", idThread)) == -1)
    {
        return 5;
    }

//Ouverture de la file du thread pour qu'il puisse recevoir des messages
    if((id_file = msgget(cle_thread, 0600|IPC_CREAT)) == -1)
    {
        return 5;
    }

//Insérer le thread dans la table des abonnés
    tab_abonnes[nombre_abonne].id_abonne = idThread;
    tab_abonnes[nombre_abonne].nbre_messages = 0;
    tab_abonnes[nombre_abonne].id_file_desc = id_file;

//Incrémenter le nombre des abonnés
    nombre_abonne++;
    pthread_mutex_unlock(&_mutex);

    return 0;
}
//fonction de desabonnement d'un thread utilisateur
int desaboMsg(pthread_t idThread)
{
    int i=0, flag =0, posThread, messagesPerdus = 0, debug;

    pthread_mutex_lock(&_mutex); //Prend le mutex

    if (test_gestionnaire() == 1)
    {
        pthread_mutex_unlock(&_mutex);
        return 1;
    }

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


//Le thread ne s'est pas abonné
    if(flag == 0)
    {
        return 2;
    }

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


//Décrementer le nombre d'abonnés dans la table
    nombre_abonne--;

    pthread_mutex_unlock(&_mutex);

    return messagesPerdus;
}
//fonction d'envoi de message
int sendMsg(pthread_t dest, pthread_t exp, char *msgEnvoi)
{
    message messageSent; //la structure du message envoyée
    int i, flagDest, flagExp, posDest, ret_mutex_lock, ret_mutex_unlock;

    ret_mutex_lock = pthread_mutex_lock(&_mutex); //Prend le mutex

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
        return 2;
    }

//Verification si la boite du destinataire est pleine
    if(tab_abonnes[posDest].nbre_messages >= taille_max_boite)
    {
        return 4;
    }

    messageSent.destinataire = dest;
    messageSent.expediteur = exp;
    char new_message[taille_message + 1];
    messageSent.msg= new_message;
    strncpy(messageSent.msg, msgEnvoi, taille_message); //Limite le message à la taille maximale définie lors de l'init"


    //Envoi du message dans la file du thread gestionnaire
    if(msgsnd(id_file_montante, &messageSent, sizeof(messageSent),IPC_NOWAIT)==-1)
    {
        return 5;
    }
//Incrémente la boite aux lettres du thread destinataire
    tab_abonnes[posDest].nbre_messages++;

    pthread_cond_broadcast(&_var_cond);
    ret_mutex_unlock = pthread_mutex_unlock(&_mutex);

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
        return "erreur : communication";
    }

    //Ouverture ou creation de la file descendante du thread
    if((id = msgget(cle_thread, 0600|IPC_CREAT))==-1)
    {
        return "erreur : communication";
    }

    i = 0;
    while(i<nbre_msg_demande) //là on permet la recuperation des messages par le thread
    {
        //Recuperation des messages dans la boite aux lettres
        ret_msgrcv = msgrcv(id, &message_recu, sizeof(message_recu),0,IPC_NOWAIT);


        if(ret_msgrcv == -1)
        {
            pthread_cond_wait(&_var_cond_rcv, &_mutex);
        }
        else{
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


    //while(flag_fermeture == 0)
    pthread_mutex_lock(&_mutex);

    while(flag_gestionnaire == 1)
    {
    //Récupère le message à transmettre (dans la file montante)
            retour_msgrcv = msgrcv(id_file_montante,&msgRecu,sizeof(msgRecu),0,IPC_NOWAIT);

            if(retour_msgrcv==-1)
            {
                pthread_cond_wait(&_var_cond, &_mutex);
                msgrcv(id_file_montante,&msgRecu,sizeof(msgRecu),0,IPC_NOWAIT);
            }


            //Incrémenter le compteur de message du thread destinataire et récupère l'id de sa file
            i=0, flag=0;
            while(flag == 0 && i<nombre_abonne)
            {
                if(tab_abonnes[i].id_abonne == msgRecu.destinataire)
                {
                    flag = 1;
                }
                i++;
            }

            if(flag==1)//Le destinataire fait parti des abonnés
            {
                cle_dest = ftok("projet_systeme.c", msgRecu.destinataire);

//Ouverture de la file du thread destinataire (si non ouverte)
                if((idDest = msgget(cle_dest, 0600|IPC_CREAT)) == -1)
                {//Rien du tout
                }

//Envoi du message dans la file du thread destinataire
                if(msgsnd(idDest, &msgRecu, sizeof(msgRecu),IPC_NOWAIT)==-1)
                {//Toujours rien
                }

                pthread_cond_signal(&_var_cond_rcv);
            }

        sleep(1);
    }

    pthread_mutex_unlock(&_mutex);

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
