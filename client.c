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


#define couleur(param) printf("\033[%sm",param)

char chemin_client[40] = "tmp/chat/";
char chemin_serveur[] = "tmp/chat/0";

struct client
{
int idclient;
int taille_message;
char *message;
};

void affichage(void){
 time_t t = time(NULL);         //Gestion du temps
 struct tm *tt = gmtime(&t);    //Gestion du temps

 couleur("0");printf("[%d:%d:%d] ",tt->tm_hour,tt->tm_min,tt->tm_sec);
 couleur("33"); printf(">> ");
}

int open_serveur(char *chemin_client)
{
 int PipeServ = open(chemin_serveur,O_WRONLY); //Ouverture du pipe serveur en écriture

 if (PipeServ < 0)
 {
   if (errno == 2) //chemin serveur non créer
   {
     int temp;
     int return_fork = fork();

     if (return_fork == -1)  perror("Error to fork");

     else if (return_fork == 0)
     {
       //printf("On est dans le fils\n");
       if (execve("serveur", NULL, NULL) < 0) perror("Error to execve");
     }

     else {
       //printf("On est dans le père\n");
       sleep(1);
       //wait(&temp);
     }
   }
   PipeServ = open(chemin_serveur,O_WRONLY);
   if ( PipeServ < 0) perror("Error to open serveur's pipe by the client");
 }
 //printf("fin de focntion fork\n");
 return PipeServ;
}

void quit_f(struct client c, char name[]){
 char msgLeft[40] = "rm tmp/chat/";
 char strPID[7];
 sprintf(strPID, "%d",c.idclient);
 strcat(msgLeft,strPID);
 system(msgLeft);             //Suppresion du pipe client
 printf("%s has left the server\n",name);
 free(c.message); //Libération de la mémoire
 exit(EXIT_SUCCESS);
}

void change_namef(struct client *d,char *name){

  char c;
  int i = 0;

  couleur("36");printf("\nEnter a NEW nickname, 100 byte maximum");
  couleur("0");printf("\n");

  for (int i = 0; i < 100; ++i) name[i] = 0;
  do{
    read(STDIN_FILENO,&c,1);
    if (c != '\n')  name[i] = c;
    ++i;
  } while(c != '\n');

  //printf("%s\n", name);
  d->message = NULL;
  char msg[] = "/nick\n";
  d->message = realloc(d->message, (strlen(name) + sizeof(msg))* sizeof(char));
  strcpy(d->message,msg);
  d->taille_message = strlen(d->message);
}
//
//---------------------------MAIN-------------------------------------------------
//
int main (int argc, char *argv[])
{
 struct client b = {0};

 b.taille_message = 0;
 b.message = malloc (b.taille_message * sizeof(char));
 b.idclient =  getpid();                  // Récupération du PID
 char strPID[7];
 for (int t = 0; t < b.taille_message; ++t) b.message[t] = 0;

//---------------------------LANCEMENT PIPE ET INTITULE---------------------------


sprintf(strPID, "%d",b.idclient);
strcat(chemin_client,strPID); //Pipe client : /tmp/chat/PID_Client

printf("----Welcome on the chat----\n");
affichage();
couleur("36"); printf("Commands : \n");
affichage();
couleur("32");printf("/quit ");
couleur("36"); printf("-- Quit the chat\n");
affichage();
couleur("32");printf("/who ");
couleur("36");printf("-- List of all users\n");
affichage();
couleur("32");printf("/nick ");
couleur("36");printf("-- Changes your nickname\n");

//---------------------------OUVERTURE DU PIPE SERVEUR----------------------------

int PipeServ = open_serveur(chemin_client);

int pipe_client = mkfifo(chemin_client,0666); //Création du pipe client auquel le serveur va se connecter
 if ( pipe_client < 0) perror("Error to create the client's pipe");

//---------------------------BOUCLE INFINIE---------------------------------------
int count = 0;
char c = 0;
char name_r[100];
char name_e[100];

int r_f = fork();

int compteur = 0;
int quit;
int change_name = 0;

while(1) {
       if (r_f < 0) perror("Error to fork");

       if (r_f > 0) //Si c'est le père, on envoie au serveur
       {
           if(write(PipeServ,&b.idclient,sizeof(int)) < 0) perror("Error to write ID client");
           if(write(PipeServ,&b.taille_message,sizeof(int))< 0) perror("Error to write message");
           if(write(PipeServ,b.message,b.taille_message)< 0) perror("Error to write message");

         if (quit == 1) quit_f(b,name_e);

         if (count == 0){
           //printf("Nom après remise à zéros : %s\n",name_e);
           couleur("36");printf("\nEnter a nickname, 100 byte maximum");
           couleur("0");printf("\n");
           do {
               read(STDIN_FILENO,&c,1);
               if (c != '\n')
               name_e[compteur] = c;
               ++compteur;
           } while(c != '\n' || compteur > 99);
             int taille =  (1 + strlen(name_e)) * sizeof(char);
             b.message = realloc(b.message,taille);  //Réallocation de mémoire
             b.message[0] = '\n';
         }
         if (count == 1){
         b.message = NULL;
         b.taille_message = 0;
         compteur = 0;
         do {
             b.taille_message += read(STDIN_FILENO,&c,1);
             b.message = realloc(b.message, (b.taille_message + strlen(name_e)) * sizeof(char));  //Réallocation de mémoire
             b.message[compteur] = c;
             ++compteur;
         } while(c != '\n' || compteur > 99);
       }
          if (compteur > 99) {
            couleur("36"); printf("Name too long, please try again");
            couleur("0"); printf("\n");
          }
         if(strcmp(b.message,"/quit\n") == 0) quit = 1;
         if(strcmp(b.message,"/nick\n") == 0) change_name = 1;

         if( change_name == 1 ) change_namef(&b,name_e);
         //printf("name : %s\n",name_e);
         change_name = 0;

         strcat(b.message,name_e);
         b.taille_message = strlen(b.message);
         count = 1;

       }
       if (r_f == 0) // Si c'est le fils, on récupère ce que le serveur envoie
       {


         char buffRec[100];
         //printf("Test\n");
         int PipeCl = open(chemin_client,O_RDONLY); //Ouverture du pipe serveur en écriture
             if (PipeCl < 0) perror("Error to open the client's pipe");

             for (int i = 0; i < 100; ++i){ //Raz du buffer de récpetion clavier
               name_r[i] = 0;
             }
             b.message = NULL;

             read(PipeCl,&name_r,sizeof(name_r));
             read(PipeCl,&b.taille_message,sizeof(b.taille_message));
             b.message = realloc(b.message, b.taille_message * sizeof(char));
             read(PipeCl,b.message,b.taille_message);

             time_t toto = time(NULL);           //Gestion du temps
             struct tm *tata = gmtime(&toto);    //Gestion du temps

             affichage();
             couleur("33");printf(" [%s] ",name_r);
             couleur("0"); printf("%s\n",b.message);

       }
}

return 0;
}


