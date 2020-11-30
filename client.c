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
#include <fcntl.h>


#define couleur(param) printf("\033[%sm",param)

char chemin_client[40] = "tmp/chat/";
char chemin_serveur[] = "tmp/chat/0";

struct client
{
int idclient;
int taille_message;
char *message;
};

void affichage(void);
int open_serveur(char *chemin_client);
void quit_f(struct client c, char name[]);
void quit_signal(int sig);
void quit_f(struct client c, char name[]);
void change_name_f(struct client *d,char *name);
void private_f(struct client *customer);
void writef(int PipeServ,struct client customer);
void first_time(struct client *customer,char *name_e);
void all_time(struct client *customer, char *name_e);

//--------------------------------------------------------------------------------
//---------------------------MAIN-------------------------------------------------
//--------------------------------------------------------------------------------
int main (int argc, char *argv[])
{
 signal(SIGINT,quit_signal);
 struct client customer = {0};

 customer.taille_message = 0;               //Affecte une taille par défaut de 0
 customer.message = malloc (customer.taille_message * sizeof(char));//Allocation mémoire par défaut nulle
 customer.idclient =  getpid();             // Récupération du PID
 customer.message = NULL;                   // Buffer messager par défaut vide

 char strPID[7];                            // Création d'un buffer qui stockera le PID
 sprintf(strPID, "%d",customer.idclient);   //Récupération du PID stocké dans le buffer préalablement créer
 strcat(chemin_client,strPID);              //Pipe client : /tmp/chat/PID_Client

//---------------------------MENU D'ACCUEIL----------------------------------------

printf("----Welcome on the chat----\n");
affichage();
couleur("36"); printf("Commands : \n");
affichage();
couleur("32");printf("/quit ");
couleur("36"); printf("    -- Quit the chat\n");
affichage();
couleur("32");printf("/who ");
couleur("36");printf("     -- List of all users\n");
affichage();
couleur("32");printf("/nick ");
couleur("36");printf("    -- Changes your nickname\n");
affichage();
couleur("32");printf("/private  ");
couleur("36");printf("-- Send a private message to somebody\n");

//---------------------------OUVERTURE DU PIPE SERVEUR----------------------------

int PipeServ = open_serveur(chemin_client); //Ouverture du pipe serveur, appel à la fonction dédié

int pipe_client = mkfifo(chemin_client,0666);//Création du pipe client auquel le serveur va se connecter
if ( pipe_client < 0) perror("Error to create the client's pipe");

//---------------------------DECLARATION DES VARIABLES----------------------------
int count = 0;
char name_r[100];
char name_e[100];

/*-- Variables de test des commandes /quit et /nick --*/
int quit;                                   //Si la variable est à 1 -> quitter le processus
int change_name = 0;                        //Si la variable est à 1 -> changer de nom
int private = 0;

int r_f = fork();
/* Séparation des fonctionnalités lecture/écriture à l'aide d'un fork
    •Processus père : écrivain
    .Processus fils : lecteur
*/


//---------------------------BOUCLE INFINIE---------------------------------------
while(1) {
       if (r_f < 0) perror("Error to fork");
                  /*-- PERE : ecrivain --*/
       if (r_f > 0)
       {
           writef(PipeServ,customer);       //Fait appel à la fonction d'écriture

         if (quit == 1) quit_f(customer,name_e);//Fait appel à la fonction quitter

/*Si c'est la première fois que la boucle est faite, on récupère le pseudo*/
         if (count == 0) first_time(&customer,name_e);
/*Sinon on récupère le message*/
         if (count == 1)all_time(&customer,name_e);

/*-- Test afin de vérifier si le msg reçu est une des deux commandes--*/
         if(strcmp(customer.message,"/quit\n") == 0) quit = 1;
         if(strcmp(customer.message,"/nick\n") == 0) change_name = 1;
         if(strcmp(customer.message,"/private\n") == 0) private = 1;

         if( change_name == 1 ) change_name_f(&customer,name_e); //Fait appel à la fonction changer de nom
         change_name = 0;
         if( private == 1) private_f(&customer);
         private = 0;

         customer.message = realloc(customer.message, (customer.taille_message + strlen(name_e)) * sizeof(char));  //Réallocation de mémoire
         strcat(customer.message,name_e);   //Concatene le pseudo au msg
         customer.taille_message = strlen(customer.message);//Récupération de la taille du msg
         //printf("Msg : %s\n",customer.message);
         count = 1;
        }
                  /*-- FILS : lecteur --*/
       if (r_f == 0) // Si c'est le fils, on récupère ce que le serveur envoie
       {
         if (getppid() == 1) exit(EXIT_SUCCESS);
         int PipeCl = open(chemin_client,O_RDONLY); //Ouverture du pipe serveur en écriture
             if (PipeCl < 0) perror("Error to open the client's pipe");

             for (int i = 0; i < 100; ++i){ //Raz du buffer de réception clavier
               name_r[i] = 0;
             }
             customer.message = NULL;       //Raz du msg

            read(PipeCl,&name_r,sizeof(name_r));
            read(PipeCl,&customer.taille_message,sizeof(customer.taille_message));
            customer.message = realloc(customer.message, customer.taille_message * sizeof(char));
            read(PipeCl,customer.message,customer.taille_message);

            private = 0;
            if (strcmp(customer.message,"/private") == 0)
            {
              read(PipeCl,&customer.taille_message,sizeof(customer.taille_message));
              customer.message = realloc(customer.message, customer.taille_message * sizeof(char));
              read(PipeCl,customer.message,customer.taille_message);
              private = 1;
            }
            printf("\33[2K\r");

             time_t toto = time(NULL);           //Gestion du temps
             struct tm *tata = gmtime(&toto);    //Gestion du temps

               affichage();
               couleur("33");printf(" [%s] ",name_r);
              if( private == 0) couleur("0");
              else couleur("35");
                printf("%s\n",customer.message);
                couleur("0");
       }
}

return 0;
}


//--------------------------------------------------------------------------------
//---------------------------FONCTIONS--------------------------------------------
//--------------------------------------------------------------------------------


/*-- Permet d'afficher sur le terminal en différentes couleurs --*/
/*
  •Affiche [heure:min:sec] en blanc
  •Affiche >> en jaune
*/
void affichage(void)
{
 time_t t = time(NULL);         //Gestion du temps
 struct tm *tt = gmtime(&t);    //Gestion du temps

 couleur("0");printf("[%d:%d:",tt->tm_hour,tt->tm_min);
 if( tt->tm_sec < 10) printf("0%d] ", tt->tm_sec);
 else printf("%d] ", tt->tm_sec);
 couleur("33"); printf(">> ");
}

/*-- Permet d'ouvrir le pipe serveur --*/
/*
    •Ouvre le pipe serveur si ce denrier est lancé
    •Execute le serveur et ouvre le pipe
*/
int open_serveur(char *chemin_client)
{
 int PipeServ = open(chemin_serveur,O_WRONLY); //Ouverture du pipe serveur en écriture

 if (PipeServ < 0 && errno == 2){// Si l'ouvertur est un échec, alors le fichier n'est pas créer -> le serveur n'est pas lancé
   //if (errno == 2){ //chemin serveur non créer
     int temp;
     int return_fork = fork();

     if (return_fork == -1) perror("Error to fork");
     else if (return_fork == 0) {
       setsid();
       if(execve("serveur", NULL, NULL) < 0) perror("Error to execve");  //On sleep le fils afin que le serveur ait le temps de se lancer
     }
     else sleep(1);
   PipeServ = open(chemin_serveur,O_WRONLY); //On ouvre le pipe fraichement créer
   if ( PipeServ < 0) perror("Error to open serveur's pipe by the client");
 }
 return PipeServ; //On retourne le FD du pipe ouvert
}

/*-- Permet de quitter proprement le client si on envoie la commande /quit --*/
/*
    •Supprime le pipe client
    •Quitte le processus à l'aide de exit(EXIT_SUCCESS)
*/
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

void quit_signal(int sig){
  exit(EXIT_SUCCESS);
}
/*-- Permet de changer de nom dès la réception de la commande /nick --*/
/*
    •Affiche en bleu d'entrer un nouveau nom
    •Boucle do while afin de récupérer le nouveau nom
*/
void change_name_f(struct client *d,char *name){

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
  d->message = NULL;
  char msg[] = "/nick\n";
  d->message = realloc(d->message, (strlen(name) + sizeof(msg))* sizeof(char));
  strcpy(d->message,msg);
  d->taille_message = strlen(d->message);
}

/*-- Permet d'envoyet un message privé à une personne --*/
/*
    •Demande le nom de la personne cible
    •Demande le message à envoyer
    •Ce qui est envoyé au serveur : /private\nPersonneCible\Message\nPersonneEmettrice
*/
void private_f(struct client *customer){

  couleur("36");printf("\nWhose do you want to send a private message ?");
  couleur("0");printf("\n");

  char name[100];
  char c;
  int indice = 0;
  char msg[100];

  for(int i = 0; i < 100; ++i) {
    name[i] = 0;
    msg[i] = 0;
  }
  do {
    read(STDIN_FILENO,&c,1);
    name[indice] = c;
    ++indice;
  }while(c != '\n');

  couleur("36");printf("\nEnter your private message");
  couleur("0");printf("\n");

  indice = 0;

  do {
    read(STDIN_FILENO,&c,1);
    msg[indice] = c;
    ++indice;
  }while(c != '\n');

  customer->message = realloc(customer->message, (strlen(name) + sizeof(customer->message) + strlen(msg) )* sizeof(char));
  strcat(customer->message,name);
  strcat(customer->message,msg);
  customer->taille_message = strlen(customer->message);
}

/*-- Permet d'écire sur le pipe --*/
/*
    •L'ID client
    •La taille du message
    •Le message + le pseudo : ce qui est envoyé est de la forme "message\npseudo"
*/
void writef(int PipeServ,struct client customer){
  if(write(PipeServ,&customer.idclient,sizeof(int)) < 0) perror("Error to write ID client");
  if(write(PipeServ,&customer.taille_message,sizeof(int))< 0) perror("Error to write message");
  if(write(PipeServ,customer.message,customer.taille_message)< 0) perror("Error to write message");
}

/*-- Récupère le pseudo sur l'entrée standard --*/
/*
    •Boucle do while (lecteur caractère par caractère)
    •Appel à read pour lire l'entrée standard
    •Rajoute un message nul avant le pseudo
*/
void first_time(struct client *customer,char *name_e){
  int compteur = 0;
  char c = 0;

  for (int i = 0; i < 100; ++i) name_e[i] = 0;
  couleur("36");printf("\nEnter a nickname, 100 bytes maximum");
  couleur("0");printf("\n");
  do {
      read(STDIN_FILENO,&c,1);
      if (c != '\n')
      name_e[compteur] = c;
      ++compteur;
  } while(c != '\n' || compteur > 99);
    int taille =  (1 + strlen(name_e)) * sizeof(char);
    customer->message = realloc(customer->message,taille);  //Réallocation de mémoire
    customer->message[0] = '\n';
}

/*-- Récupère le message sur l'entrée standart --*/
/*
    •Boucle do while (lecteur caractère par caractère)
    •Appel à read pour lire l'entrée standard
*/
void all_time(struct client *customer, char *name_e){
  int compteur = 0;
  char c = 0;

  customer->message = NULL;
  customer->taille_message = 0;
  compteur = 0;
  do {
      customer->taille_message += read(STDIN_FILENO,&c,1);
      customer->message = realloc(customer->message, (customer->taille_message + strlen(name_e)) * sizeof(char));  //Réallocation de mémoire
      customer->message[compteur] = c;
      ++compteur;
  } while(c != '\n');
}
