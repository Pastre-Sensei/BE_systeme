#include "projet_systeme.h"


void * fonc_thread1 (void * arg){

    int abo_retour;
    pthread_t dest;
    sleep(1);
    abo_retour = aboMsg(pthread_self());
    printf("thread 1 : %d\n", abo_retour);

}

void * fonc_thread2 (void * arg){

    int abo_retour;
    sleep(2);
    abo_retour = aboMsg(pthread_self());
    printf("thread 2 : %d\n", abo_retour);

}



int main(){

    pthread_t thread1, thread2;

    initMsg(3,50,50);
    printf("blabla\n");

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



