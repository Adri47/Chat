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

char chemin[] = "tmp/chat/";
char chemin_serveur[] = "tmp/chat/0";
char PathClient[40];


struct serveur
{
int idclient;
int taille_message;
char *message;
char name[100];

};


void signal_sigint(int sig){
 printf("\nSignal received : %d\n",sig);
 printf("Server OFF");
 exit(EXIT_SUCCESS);          //Quitte le programme
}

//
//---------------------------MAIN-------------------------------------------------
//

int main (int argc, char *argv[])
{
signal(SIGINT,signal_sigint);
//---------------------------GESTION DU REPERTOIRE--------------------------------

system("rm -r tmp/");        //Suppresion du repertoires tmp/
system("mkdir -p tmp/chat"); //Création des repetoire tmp/chat

//---------------------------LANCEMENT DU PIPE SERVEUR----------------------------
int return_mkfifo = mkfifo(chemin_serveur, 0777);
    if (return_mkfifo < 0) perror("Error to create the mkfifo server");

//---------------------------OUVERTURE DU PIPE SERVEUR----------------------------
int PipeServ = open(chemin_serveur,O_RDONLY); //Ouverture en lecture
    if (PipeServ < 0) perror("Error lauching server");
printf("Server ON\n");



struct serveur b[MAX_CLIENTS];
//char name[MAX_CLIENTS][100];
char c = 0;
int i = 0;
//int t = 0;
int count = 0;
int currentClient = 0;
int numberClient = 0;
int temporaire = 0;
char strPID[7];
int newclient = 0;
char oldname[100];
int l = 0;
int who =0;
int first_time = 0;
char buff[100];

for (int g = 0; g < numberClient; ++g){
    b[g].taille_message = 0;
    for (int hh = 0; hh < 100; ++hh) b[g].name[hh] = 0;
}


while(1){
//---------------------------SERVEUR RECEPTION------------------------------------
    printf("------SERVEUR RECEPTION------\n");
    for (int g = 0; g < numberClient; ++g){  //Remise à zéros des buffers
      i = 0;
      who = 0;
      count = 0;
      newclient = 0;
    for (int k = 0 ; k  < b[g].taille_message; ++k){
          b[g].message[k] = 0;
      }
    }


//---------------------------GESTION DES ID RECUES--------------------------------
    if (numberClient == 0){
      if (read(PipeServ,&b[currentClient].idclient,sizeof(b[currentClient].idclient)) < 0) perror("Error to read ID client");
    }
    else {
      if (read(PipeServ,&temporaire,sizeof(int)) < 0) perror("Error to read ID client");

        for(int y = 0; y < numberClient; ++y){
          printf("PID client %d : %d -- PID reçu : %d\n",y,b[y].idclient,temporaire);
          if ( b[y].idclient == temporaire ){ //Identification du client qui parle
            currentClient = y;                //Sauvegarde de l'indice du client qui discute
            newclient = 1;                    //Ce n'est pas un nouveau client
            break;
          }
        }
        if(newclient == 0){
          for(int y = 0; y < numberClient; ++y){//S ic'est un nouveau client, on regarde si y'a un place dans libre dans le tableau, sinon on incrémente l'indice
              if (b[y].idclient == 1){
                currentClient = y;
                break;
              }
              else currentClient = numberClient;
          }
        }
        b[currentClient].idclient = temporaire; //On lie le PID reçu au client actuel.
    }
    printf("currentClient : %d\n",currentClient);

//---------------------------LECTURE----------------------------------------------
    if (read(PipeServ,&b[currentClient].taille_message,sizeof(b[currentClient].taille_message)) < 0) perror ("Error to read taille_message");
    if(b[currentClient].taille_message == 0) b[currentClient].message = malloc(b[currentClient].taille_message * sizeof(char));
    else b[currentClient].message = realloc(b[currentClient].message, b[currentClient].taille_message * sizeof(char));
    printf("PID du client actuel : %d\n",b[currentClient].idclient);

    if (b[currentClient].taille_message == 0){
        printf("Message de bienvenue\nPID : %d\n",b[currentClient].idclient);
        ++numberClient;
        first_time = 1;
        printf("Nombre de client : %d\n",numberClient);
    }
    if (b[currentClient].taille_message != 0) {
      do {
          read(PipeServ,&c,1);
          //printf("C : %c -- i : %d\n",c,i);
          if ( c == '\n') count = 1;
          if (count == 0 && i != b[currentClient].taille_message){
            b[currentClient].message[i] = c;
            ++i;
            l = 1;
          }
          if (count == 1 && strcmp(b[currentClient].message,"/nick") == 0 && l == 1){
            //printf("OK\n");
            l = 0;
            for( int p = 0; p < 100; ++p ) oldname[p] = 0;
            strcpy(oldname,b[currentClient].name);
            printf("Oldname : %s -- Name : %s\n",oldname,b[currentClient].name);
            first_time = 1;
            for( int y = 0; y < 100; ++y) b[currentClient].name[y] = 0;
          }
          if(count == 1 && i != b[currentClient].taille_message && first_time == 1) {
            read(PipeServ,b[currentClient].name, b[currentClient].taille_message-i);
            i = b[currentClient].taille_message;
            first_time = 0;
          }
           if(count == 1 && i != b[currentClient].taille_message && first_time == 0) {
            read(PipeServ,buff, b[currentClient].taille_message-i);
            i = b[currentClient].taille_message;
          }
      } while(i != b[currentClient].taille_message);


      printf("[%s]\nTaille msg : %d\nMessage reçu : %s\n",
                  b[currentClient].name,
                  b[currentClient].taille_message,
                  b[currentClient].message);
    }

//---------------------------GESTION DES COMMANDES--------------------------------
    if(strcmp(b[currentClient].message,"/quit") == 0){ //Si msg /quit
      b[currentClient].idclient = 1; //On met la variable à '1'
    }
    if(strcmp(b[currentClient].message,"/who") == 0 ) who = 1;
//---------------------------SERVEUR EMISSION-------------------------------------
    if (b[currentClient].taille_message != 0){
      printf("\n------SERVEUR EMISSION------\n");


      for(int h = 0; h < numberClient; ++h){
        for(int l = 0; l < 40; ++l){
          PathClient[l] = 0;
        }
        if (who == 1){
          strcpy(PathClient,chemin);
          sprintf(strPID, "%d",b[currentClient].idclient);
          strcat(PathClient,strPID);
          printf("Path : %s\n",PathClient);
        }
        else {
          strcpy(PathClient,chemin);
          sprintf(strPID, "%d",b[h].idclient);
          strcat(PathClient,strPID);
          printf("Path : %s\n",PathClient);
        }


        int PipeCl = open(PathClient,O_WRONLY); //Ouverture du pipe serveur en écriture
            if (PipeCl < 0) perror("Error to open the client's pipe");


            if (strcmp(b[currentClient].message,"\0") == 0) { //Si le msg est null, alors msg de bienvenue
              if(write(PipeCl,b[currentClient].name,sizeof(b[currentClient].name)) < 0) perror("Error to write the name");
              char t[] = "has joigned the serveur";
              int taille = strlen(t);
              if (write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (welcome)");
              if (write(PipeCl,t,taille) < 0) perror("Error to write the message (welcome)");
            }
            else if (strcmp(b[currentClient].message,"/quit") == 0) {//Si c'est le message pour quitter le chat
              if(write(PipeCl,b[currentClient].name,sizeof(b[currentClient].name)) < 0) perror("Error to write the name");
              char t[] = "has left the server";
              int taille = strlen(t);
              if(write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (quit)");
              if(write(PipeCl,t,taille) < 0) perror("Error to write the message (quit)");
              --numberClient;
            }
            else if (strcmp(b[currentClient].message,"/nick") == 0) {// Si demande de changement de nom
              if(write(PipeCl,oldname,sizeof(oldname)) < 0) perror("Error to write the name");
              char t[140] = "has changed is name in ";
              strcat(t,b[currentClient].name);
              int taille = strlen(t);
              if(write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (quit)");
              if(write(PipeCl,t,taille) < 0) perror("Error to write the message (quit)");
            }
            else if (strcmp(b[currentClient].message,"/who") == 0 ){
              printf("Nom : %s\n",b[h].name);
              if(write(PipeCl,b[h].name,sizeof(b[currentClient].name)) < 0) perror("Error to write the name");
              char t[] = "is online";
              int taille = strlen(t);
              if(write(PipeCl,&taille,sizeof(int)) < 0) perror("Error to write the size (quit)");
              if(write(PipeCl,t,taille) < 0) perror("Error to write the message (quit)");
            }
            else if (strcmp(b[currentClient].message,"\0") != 0){
              if(write(PipeCl,b[currentClient].name,sizeof(b[currentClient].name)) < 0) perror("Error to write the name");
              if (write(PipeCl,&b[currentClient].taille_message,sizeof(b[currentClient].taille_message)) < 0) perror("Error to write the size");
              if (write(PipeCl,b[currentClient].message,b[currentClient].taille_message) < 0) perror("Error to write the message");

            }
      }
    }
}


return 0;
 
}


