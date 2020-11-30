# Chat

Pour compiler le projet :
>> make chat

Pour lancer le serveur puis un client :
>>./serveur
>>./client

Pour lancer le serveur directement par le client :
>>./client

Exécuter autant de fois l’exécutable du client pour ajouter un client.

Une fois le projet lancer : 
    - Choisir son Pseudo (exemple : Adrien)
    - Envoyer un message (exemple : Bonjour à tous !)
    - Utiliser les commandes disponibles si vous le souhaitez (voir explication ci-dessous)

Commandes utiles :
    - /quit : permet au client de quitter le chat
    - /who : voir les personnes qui sont sur le chat
    - /nick : changer son pseudo 
    - /private : envoyer un message à un autre client dans une conversation privée 

Arrêter le chat :
    Envoyer un signal SIG_INT, avec la commande kill, au processus du client qui a ouvert le serveur
    >> kill -s SIG_INT [PID]
