# Shell en C - Gestionnaire de Commandes

Ce projet consiste à développer un shell en langage C avec plusieurs fonctionnalités progressivement intégrées. Vous pouvez travailler en groupe de 1 ou 2 personnes. Les différentes étapes comprennent la création d'un shell traitant des commandes sans arguments, l'ajout de la gestion des arguments, la redirection de la sortie standard, la gestion des tubes (pipes), et la gestion des signaux.

## Exercice 1 - Version Dépouillée

- Écrire un shell qui traite uniquement des commandes ne possédant pas d'arguments.
- Le shell doit afficher une invite de commande ('$' suivi d'un espace).
- Attendre que l'utilisateur entre une commande.
- Se dupliquer pour exécuter cette commande.
- Attendre la terminaison du nouveau processus.
- Si la commande est "exit", le shell doit s'arrêter.

## Exercice 2 - Traitement des Arguments

- Écrire une fonction `int parse_line(char *s, char **argv[])` qui découpe la chaîne `s` en fonction des espaces.
- Les mots séparés par des espaces doivent être stockés dans le tableau `*argv`. La dernière case du tableau doit contenir `NULL` pour marquer la fin du tableau.
- Exemple : Si la chaîne `s` est "commande -v toto tata", le tableau doit être rempli comme suit : `commande -v toto tata NULL`.

## Exercice 3 - Intégration des Arguments

- Modifier le shell pour prendre en compte les arguments lors de l'exécution des commandes.

## Exercice 4 - Redirection de la Sortie Standard

- Permettre au shell de traiter une redirection de la sortie standard vers un fichier.
- Le caractère '>' sera toujours donné en avant-dernier argument, et le fichier en dernier argument.

## Exercice 5 - Tubes Simples

- Permettre au shell de traiter une version simplifiée du chaînage de commandes.
- Si le dernier argument d'une commande est le caractère '|', le shell doit attendre une deuxième commande et rediriger la sortie standard de la première sur l'entrée standard de la seconde.

## Exercice 6 - Gestion des Signaux

- Faire en sorte que le shell ignore le signal SIGINT lorsqu'on appuie sur CTRL+C dans la console.
- Les programmes lancés par le shell ne doivent pas ignorer ce signal.
