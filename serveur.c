#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <time.h>


#define MAX_CLIENTS 50
#define MAX_NAME 100

char chemin[] = "tmp/chat/";
char chemin_serveur[] = "tmp/chat/0";
char PathClient[40];


struct serveur
{
int idclient;
int taille_message;
char *message;
char name[MAX_NAME];

};

struct serveur server[MAX_CLIENTS];         //Créer un tableau de type struct serveur de MAX_CLIENTS
int numberClient = 0;                       //Nombre de clients totaux connectés

void raz();
int target_identification(char *target);
int name_taken();
void signal_sigint(int sig);
int gestion_ID(int PipeServ,int currentClient);
void read_f(int PipeServ,int currentClient);
void name_taken_f(int PipeCl, int h, int currentClient);
void private_f(int PipeCl,int currentClient, int target_ID);
void envoi_bienvenue(int PipeCl,int currentClient);
void quit_f(int PipeCl, int currentClient);
void who_f(int PipeCl, int currentClient, int h);
void envoi_classique(int PipeCl,int currentClient);

//--------------------------------------------------------------------------------
//---------------------------MAIN-------------------------------------------------
//--------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
signal(SIGINT,signal_sigint);               //Gère le signal SINGINt (CTRL-C)
printf("SERVEUR PID : %d\n",getpid());
//---------------------------GESTION DU REPERTOIRE--------------------------------

system("rm -r tmp/");                       //Suppresion du repertoires tmp/
system("mkdir -p tmp/chat");                //Création des repetoire tmp/chat

//---------------------------LANCEMENT DU PIPE SERVEUR----------------------------
int return_mkfifo = mkfifo(chemin_serveur, 0777);//Créer le pipe serveur
    if (return_mkfifo < 0) perror("Error to create the mkfifo server");

int PipeServ = open(chemin_serveur,O_RDONLY);//Ouverture en lecture
    if (PipeServ < 0) perror("Error lauching server");


//---------------------------DECLARATION DES VARIABLES----------------------------
int currentClient = 0;                      //Numéros du client actuel (celui qui dialogue)

int NomPrit = 0;                            // 0 si le nom du client currentClient est libre, 1 sinon
int first_time = 0;

char oldname[MAX_NAME];                     //Sauvegarde l'ancien nom du currentClient
char buff[100];                             //Buffer
char target[MAX_NAME];                      //Buffer de réception du nom du client cible par la commande /private


char c = 0;                                 //Caractère lu
int indice_msg = 0;                         //Indice du tableau customer[currentClient].message[indice_msg]
int count = 0;                              // 0 si on a pas atteint le caractère '\n' en lisant le msg reçu, 1 sinon

char strPID[7];

int l = 0;                                  // ??
int who = 0;                                // 0 si le msg reçu est différent de /who, 1 sinon
int private = 0;                            // 0 si le msg reçu est différent de /private, 1 sinon
int target_ID = 0;                          // PID du client target par la commande /private


for (int g = 0; g < MAX_CLIENTS; ++g){      //Met à zéros les le buffer de réception (message et name)
    server[g].taille_message = 0;
    for (int hh = 0; hh < MAX_NAME; ++hh) server[g].name[hh] = 0;
}

//---------------------------BOUCLE INFINIE---------------------------------------
while(1){
            /*-- RECEPTION --*/

    raz();                                  //Fait appel à la fonction raz()
            /*-- Remise à zéro des variables --*/
    indice_msg = 0;
    who = 0;
    count = 0;


//---------------------------GESTION DES ID RECUES--------------------------------

    currentClient = gestion_ID(PipeServ,currentClient);//Fait appel à la fonction gestion_ID

//---------------------------LECTURE----------------------------------------------

    read_f(PipeServ,currentClient);

//---------------------------RECUPERATION DES MSG----------------------------------
    if (server[currentClient].taille_message == 0){ //Si c'est un msg de taille nul
        ++numberClient;
        first_time = 1;
    }
    if (server[currentClient].taille_message != 0) {
      while(indice_msg != server[currentClient].taille_message){
          read(PipeServ,&c,1);
          if ( c == '\n') count = 1;                                            //Si c'est le caractère lu égale à '\n'
          if (count == 0 && indice_msg != server[currentClient].taille_message){//Si on n'a pas atteint le caractère déléminateur '\n', on stocke le caractère reçu dans le msg
            server[currentClient].message[indice_msg] = c;
            ++indice_msg;
            l = 1;
          }
          if (count == 1 && strcmp(server[currentClient].message,"/nick") == 0 && l == 1){ //Si on envoie /nick
            l = 0;
                  for( int p = 0; p < MAX_NAME; ++p ) oldname[p] = 0;// Changement de nom client
                  strcpy(oldname,server[currentClient].name);
                  first_time = 1;
                  for( int y = 0; y < MAX_NAME; ++y) server[currentClient].name[y] = 0;

          }
          if (count == 1 && strcmp(server[currentClient].message,"/private") == 0 && l == 1){
              //strcpy(private_string,b.[currentClient])
              char c_target = 0;
              int indice = 0;
              private = 1;

              for (int i = 0; i < MAX_NAME; ++i) {
                target[i] = 0;
              }
              while(c_target != '\n'){
                read(PipeServ,&c_target,1);
                if(c_target != '\n')  target[indice] = c_target;
                ++indice;
              }
              count = 0;
              indice_msg = 0;
              for (int i = 0; i < 8; ++i) server[currentClient].message[i] = 0;
          }
          if(count == 1 && indice_msg != server[currentClient].taille_message && first_time == 1) { //Si on a dépassé le caractère déléminateur, on stocke le reste du pipe dans le nom client
            read(PipeServ,server[currentClient].name, server[currentClient].taille_message-indice_msg);
            indice_msg = server[currentClient].taille_message;
            first_time = 0;
          }
          if(count == 1 && indice_msg != server[currentClient].taille_message && first_time == 0) {
            read(PipeServ,buff, server[currentClient].taille_message-indice_msg);
            indice_msg = server[currentClient].taille_message;
          }
      }

    }


//---------------------------GESTION DES COMMANDES--------------------------------
    if(strcmp(server[currentClient].message,"/quit") == 0)server[currentClient].idclient = 1; //On met le PID à '1' si on reçoit la commande quit
    if(strcmp(server[currentClient].message,"/who") == 0 ) who = 1;
    //if(strcmp(b[currentClient].message,"/private") == 0 ) private = 1;

    NomPrit = name_taken();
//---------------------------SERVEUR EMISSION-------------------------------------
    if (server[currentClient].taille_message != 0 || NomPrit == 1){
      //printf("\n------SERVEUR EMISSION------\n");


      for(int h = 0; h < numberClient; ++h){

        for(int l = 0; l < 40; ++l) PathClient[l] = 0;

        if (who == 1 || NomPrit == 1){
          strcpy(PathClient,chemin);
          sprintf(strPID, "%d",server[currentClient].idclient);
          strcat(PathClient,strPID);
          //printf("Path : %s\n",PathClient);
        }
        else if (private == 1){
          target_ID = target_identification(target);

          if(target_ID == -1){
            strcpy(PathClient,chemin);
            sprintf(strPID, "%d",server[currentClient].idclient);
            strcat(PathClient,strPID);
            //printf("Path : %s\n",PathClient);
            //printf("ID de la cible : %d\n",target_ID);
          }
          else {
            strcpy(PathClient,chemin);
            sprintf(strPID, "%d",target_ID);
            strcat(PathClient,strPID);
            //printf("Path : %s\n",PathClient);
          }
        }
        else {
          strcpy(PathClient,chemin);
          sprintf(strPID, "%d",server[h].idclient);
          strcat(PathClient,strPID);
          //printf("Path : %s\n",PathClient);
        }


        int PipeCl = open(PathClient,O_WRONLY); //Ouverture du pipe serveur en écriture
            if (PipeCl < 0) perror("Error to open the client's pipe");

            if (NomPrit == 1) {
              name_taken_f(PipeCl,h, currentClient);
              break;
            }
            else if(private == 1 ){
                private_f(PipeCl,currentClient, target_ID);
                private = 0;
                break;
              }
            else if (strcmp(server[currentClient].message,"\0") == 0) envoi_bienvenue(PipeCl,currentClient);
            else if (strcmp(server[currentClient].message,"/quit") == 0) quit_f(PipeCl,currentClient);

            else if (strcmp(server[currentClient].message,"/nick") == 0) {// Si demande de changement de nom
              if(write(PipeCl,oldname,sizeof(oldname)) < 0) perror("Error to write the name");
              char t[140] = "has changed is name in ";
              strcat(t,server[currentClient].name);
              int taille = strlen(t);
              if(write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (nick)");
              if(write(PipeCl,t,taille) < 0) perror("Error to write the message (nick)");

            }
            else if (strcmp(server[currentClient].message,"/who") == 0 ) who_f(PipeCl,currentClient,h);
            else if (strcmp(server[currentClient].message,"\0") != 0) envoi_classique(PipeCl,currentClient);


      }
    }
}


return 0;
}


/*-- Fonction d'un envoi classique --*/
/*
    •Envoie un message classique à tous les clients
*/
void envoi_classique(int PipeCl,int currentClient){

  if(write(PipeCl,server[currentClient].name,sizeof(server[currentClient].name)) < 0) perror("Error to write the name");
  if (write(PipeCl,&server[currentClient].taille_message,sizeof(server[currentClient].taille_message)) < 0) perror("Error to write the size");
  if (write(PipeCl,server[currentClient].message,server[currentClient].taille_message) < 0) perror("Error to write the message");

}

/*-- Fonction d'envoi pour la commande /who --*/
/*
    •Envoie la liste des clients connectés au currentClient
*/
void who_f(int PipeCl, int currentClient, int h){

if(write(PipeCl,server[h].name,sizeof(server[currentClient].name)) < 0) perror("Error to write the name");
char t[] = "is online";
int taille = strlen(t);
if(write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (quit)");
if(write(PipeCl,t,taille) < 0) perror("Error to write the message (quit)");

}

/*-- Fonction d'envoi pour la commande /quit --*/
/*
    •Préviens tous les clients que le currentClient a quitté le serveur
*/
void quit_f(int PipeCl, int currentClient){

if(write(PipeCl,server[currentClient].name,sizeof(server[currentClient].name)) < 0) perror("Error to write the name");
char t[] = "has left the server";
int taille = strlen(t);
if(write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (quit)");
if(write(PipeCl,t,taille) < 0) perror("Error to write the message (quit)");
--numberClient;

}


/*-- Fonction d'envoi pour le message de bienvenue --*/
/*
    •Envoie un message au client
*/
void envoi_bienvenue(int PipeCl,int currentClient){

  if(write(PipeCl,server[currentClient].name,sizeof(server[currentClient].name)) < 0) perror("Error to write the name");
  char t[] = "has joigned the serveur";
  int taille = strlen(t);
  if (write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (welcome)");
  if (write(PipeCl,t,taille) < 0) perror("Error to write the message (welcome)");
}

/*-- Fonction d'envoi pour la commande private --*/
/*
    •Envoi un message au client
*/
void private_f(int PipeCl,int currentClient, int target_ID){
  char t[50];
  if (target_ID == -1) strcpy(t," is not valid name");
  else strcpy(t,"/private");

  int taille = strlen(t);

  if(write(PipeCl,server[currentClient].name,sizeof(server[currentClient].name)) < 0) perror("Error to write the name");
  if(write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (private)");
  if(write(PipeCl,t,taille) < 0) perror("Error to write the message (/private)");

  if(write(PipeCl,&server[currentClient].taille_message,sizeof(server[currentClient].taille_message)) < 0) perror("Error to write the size of message (private)");
  if(write(PipeCl,server[currentClient].message,server[currentClient].taille_message) < 0) perror("Error to write the message (private)");
}


/*-- Fonction d'envoi pour un nom déjà prit --*/
/*
    •Envoi un message au client que son nom est déjà prit
*/
void name_taken_f(int PipeCl, int h, int currentClient){

  if(write(PipeCl,server[h].name,sizeof(server[currentClient].name)) < 0) perror("Error to write the name");
  char t[] = "name already used, please changed yours (use /nick)";
  int taille = strlen(t);
  if(write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (quit)");
  if(write(PipeCl,t,taille) < 0) perror("Error to write the message (quit)");

}

/*-- Lit les informations sur le pipe --*/
/*
    •Lit la taille message
    •Alloue la mémoire (malloc et realloc) en fonction de la taille
*/
void read_f(int PipeServ,int currentClient){

  if(read(PipeServ,&server[currentClient].taille_message,sizeof(server[currentClient].taille_message)) < 0) perror ("Error to read taille_message");
  if(server[currentClient].taille_message == 0) server[currentClient].message = malloc(server[currentClient].taille_message * sizeof(char));
  else server[currentClient].message = realloc(server[currentClient].message, server[currentClient].taille_message * sizeof(char));

}

/*-- Identifie le client qui parle --*/
/*
    •Regarde s'il y'a pluseirs clients, sinon on lit et stocke directement l'ID dans le struct à l'indice 0
    •Sinon on stocke l'ID dans une variable temporaire
    •On regarde si le client s'est déjà connecté, on récupère l'indice du client et on sort de la boucle
    •Sinon, on stocke l'ID dans la première casse libre (PID = 1), et on retourne l'indice du client
*/
int gestion_ID(int PipeServ,int currentClient){

  int newclient = 0;                        //Permet de savoir si le client actuel est nouveau (0 : new client, 1 sinon)
  int PID_temporaire = 0;                   //Sauvegarde le PID le temps d'identifier le currentClient

  if (numberClient == 0){
    if (read(PipeServ,&server[currentClient].idclient,sizeof(server[currentClient].idclient)) < 0) perror("Error to read ID client");
  }
  else {
    if (read(PipeServ,&PID_temporaire,sizeof(int)) < 0) perror("Error to read ID client");

      for(int y = 0; y < numberClient; ++y){
        if ( server[y].idclient == PID_temporaire ){  //Identification du client qui parle
          currentClient = y;                          //Sauvegarde de l'indice du client qui discute
          newclient = 1;                              //Ce n'est pas un nouveau client
          break;
        }
      }
      if(newclient == 0){
        for(int y = 0; y < numberClient; ++y){//Si c'est un nouveau client, on regarde si y'a un place dans libre dans le tableau, sinon on incrémente l'indice
            if (server[y].idclient == 1){
              currentClient = y;
              break;
            }
            else currentClient = numberClient;
        }
      }
      server[currentClient].idclient = PID_temporaire; //On lie le PID reçu au client actuel.
  }
  return currentClient;
}

/*-- Fonction suite à une interruption (CTRL-C) --*/
/*
    •Quitte le processus
*/
void signal_sigint(int sig){

char strPID[7];
for(int l = 0; l < 40; ++l) PathClient[l] = 0;
for(int i = 0; i < numberClient; ++i){
    strcpy(PathClient,chemin);
    sprintf(strPID, "%d",server[i].idclient);
    strcat(PathClient,strPID);

    int PipeCl = open(PathClient,O_WRONLY); //Ouverture du pipe serveur en écriture
    if(write(PipeCl,"SERVER is down",sizeof("SERVER is down")) < 0) perror("Error to write the name (quit function)");

    kill(server[i].idclient,SIGINT);
  }
 //printf("\nSignal received : %d\n",sig);
 system("rm -r tmp/");
 printf("Server OFF");
 exit(EXIT_SUCCESS);          //Quitte le programme
}

/*-- Regarde si tous les noms sont uniques --*/
/*
    •Deux boucles for pour balayer tous les élements du tableau
    •Retourne 0 si tous les nom sont uniques
    •Retourne 1 si deux noms sont identiques
*/
int name_taken(){
  for(int i = 0; i < numberClient; ++i){
    for(int j = 0; j < numberClient; ++ j){
      if(j != i && strcmp(server[i].name,server[j].name) == 0) return 1;
    }
  }
  return 0;
}

/*-- Identifie le client cible --*/
/*
    •Compare le nom des tous les clients avec le client cible
    • Retourne le PID du client cible si ce dernier existe
    • -1 sinon
*/
int target_identification(char *target){
    for(int i = 0; i < numberClient; ++i){
        if(strcmp(target,server[i].name) == 0) return server[i].idclient;
    }
    return -1;
}

/*-- Remise à zéro des buffers message et nom --*/
/*

*/
void raz(){
  for (int i = 0; i < numberClient; ++i){  //Remise à zéros des buffers
  for (int j = 0 ; j  < server[i].taille_message; ++j){
        server[i].message[j] = 0;
    }
  }
}
