#include <stdio.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <ctype.h>
#include <signal.h>

#define MAXLINE 1024

char **arguments;
int argc;

//Fonction qui va remplacer le comportement de SIGINT
static void handler(int signum) {}

//Code qui decoupe les arguments
int parse_line(char *s, char ***argv, char **output_file) {
    int argcount = 0;
    char *token = strtok(s, " ");
    *argv = malloc(sizeof(char *));
    if(*argv == NULL) {
        perror("Erreur allocation memoire argv");
        exit(EXIT_FAILURE);
    }

    while (token != NULL) {
        //Si c'est un >, récupère le nom du fichier
        if (strcmp(token, ">") == 0) {
            token = strtok(NULL, " ");
            *output_file = strdup(token);
            //Retire les espaces devant le nom du fichier
            while (*output_file != NULL && isspace(**output_file)) {
                (*output_file)++;
            }
        } else {
            //Nouveau token trouvé
            argcount++;
            *argv = realloc(*argv, (argcount + 1) * sizeof(char *));
            if(*argv == NULL) {
                perror("Erreur allocation mémoire");
            }
            (*argv)[argcount - 1] = strdup(token);
        }
        //Avance au prochain token
        token = strtok(NULL, " ");
    }
    //La dernière case du tableau doit contenir NULL
    (*argv)[argcount] = NULL;
    return argcount;
}

//Fonction qui libère la mémoire, sans arguments, pour atexit()
void freeExit() {
    for (int i = 0; i < argc; i++) {
        free(arguments[i]);
    }
    free(arguments);
}

//Fonction qui libère la mémoire, avec arguments, pendant l'execution
void freeMemory(int arg, char **list) {
    for (int i = 0; i < arg; i++) {
        free(list[i]);
    }
    free(list);
}

int main() {
    //Crée une action qui servira a remplacer le comportement par défaut de SIGINT
    struct sigaction act;
    act.sa_handler = handler;
    //Flag SA_RESTART pour que SIGINT ne retourne pas a son comportement par défaut une fois qu'il a été appelé
    act.sa_flags = SA_RESTART;
    //Remplace le comportement de SIGINT par la nouvelle action act
    sigaction(SIGINT, &act, NULL);

    while (1) {
        //Affiche l'"invite de commande"
        printf("$ ");
        //Récupère la commande entrée par l'utilisateur
        char command[MAXLINE];
        if (fgets(command, sizeof(command), stdin) == NULL) {
            perror("Erreur fgets, lors de la lecture de la commande");
            exit(EXIT_FAILURE);
        }

        //Supprime le saut de ligne à la fin de la commande
        command[strcspn(command, "\n")] = '\0';

        //Condition pour la commande exit
        if (strcmp(command, "exit") == 0) {
            exit(EXIT_SUCCESS);
        }

        //Obtenir les arguments et le fichier de sortie
        char *output_file = NULL;
        argc = parse_line(command, &arguments, &output_file);

        //Ajoute une fonction qui sera appelée a la fermeture du programme
        atexit(freeExit);

        //Vérifie si le dernier argument est le caractère '|'
        if (argc > 0 && strcmp(arguments[argc - 1], "|") == 0) {
            //Supprime le caractère '|' de la liste des arguments
            free(arguments[argc - 1]);
            arguments[argc - 1] = NULL;
            argc--;

            //Attendre la deuxième commande
            printf("Waiting for the second command...\n");

            //Lire la deuxième commande
            char second_command[MAXLINE];
            if (fgets(second_command, sizeof(second_command), stdin) == NULL) {
                perror("Erreur fgets, lors de la lecture de la deuxième commande");
                exit(EXIT_FAILURE);
            }

            //Supprime le saut de ligne à la fin de la deuxième commande
            second_command[strcspn(second_command, "\n")] = '\0';

            //Créer un tube (pipe)
            int pipe_fd[2];
            if (pipe(pipe_fd) == -1) {
                perror("Erreur lors de la création du tube (pipe)");
                exit(EXIT_FAILURE);
            }

            pid_t pid = fork();
            if (pid == -1) {
                perror("Erreur lors de la création du processus fils");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                //Processus fils : Redirection de la sortie standard vers l'entrée du tube
                //Fermer l'extrémité d'écriture du tube
                if (close(pipe_fd[0]) == -1) {
                    perror("Erreur fermeture d'extrémité d'écriture du tube");
                }
                dup2(pipe_fd[1], STDOUT_FILENO);
                //Fermer l'extrémité d'écriture du tube
                if (close(pipe_fd[1]) == -1) {
                    perror("Erreur fermeture d'extrémité d'écriture du tube");
                }

                //Exécution de la première commande
                if (execvp(arguments[0], arguments) == -1) {
                    perror("Erreur lors de l'exécution de la première commande");
                    exit(EXIT_FAILURE);
                }
            } else {
                //Processus père
                waitpid(pid, NULL, 0); //Attente de la fin du processus fils
                //Fermer l'extrémité d'écriture du tube
                if (close(pipe_fd[1]) == -1) {
                    perror("Erreur fermeture d'extrémité d'écriture du tube");
                }
                //Préparer les arguments pour la deuxième commande
                char **second_arguments;
                char *second_output_file = NULL;
                int second_argc = parse_line(second_command, &second_arguments, &second_output_file);

                //Créer un autre processus fils pour exécuter la deuxième commande
                pid_t second_pid = fork();
                if (second_pid == -1) {
                    perror("Erreur lors de la création du deuxième processus fils");
                    exit(EXIT_FAILURE);
                } else if (second_pid == 0) {
                    //Processus fils : Redirection de l'entrée standard depuis le tube
                    //Fermer l'extrémité d'écriture du tube
                    // if (close(pipe_fd[1]) == -1) {
                    //    perror("Erreur fermeture d'extrémité d'écriture du tube");
                    // }
                    dup2(pipe_fd[0], STDIN_FILENO);

                    //Fermer l'extrémité d'écriture du tube
                    if (close(pipe_fd[0]) == -1) {
                        perror("Erreur fermeture d'extrémité d'écriture du tube");
                    }

                    //Exécution de la deuxième commande
                    if (execvp(second_arguments[0], second_arguments) == -1) {
                        perror("Erreur lors de l'exécution de la deuxième commande");
                        exit(EXIT_FAILURE);
                    }
                } else {
                    //Processus père : Attendre la fin du deuxième processus fils
                    waitpid(second_pid, NULL, 0);
                    //Fermer l'extrémité de lecture du tube
                    if(close(pipe_fd[0]) == -1) {
                        perror("Erreur fermeture d'extrémité de lecture du tube");
                    }

                    //Libérer la mémoire des arguments de la première commande
                    freeMemory(argc, arguments);

                    //Libérer la mémoire des arguments de la deuxième commande
                    freeMemory(second_argc, second_arguments);
                }
            }
        } else {
            //Pas de chaînage de commandes, exécuter la commande normalement
            pid_t pid = fork();
            if (pid == -1) {
                perror("Erreur lors de la création du processus fils");
                exit(EXIT_FAILURE);
            } else if (pid == 0) {
                //Processus fils
                //Vérifie s'il y a une redirection de la sortie standard
                if (output_file != NULL) {
                    // Ouvre le fichier de sortie
                    int fd = open(output_file, O_WRONLY | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
                    if (fd == -1) {
                        perror("Erreur lors de l'ouverture du fichier de sortie");
                        exit(EXIT_FAILURE);
                    }
                    // Redirige la sortie standard vers le fichier
                    if (dup2(fd, STDOUT_FILENO) == -1) {
                        perror("Erreur lors de la redirection de la sortie standard");
                        if(close(fd) == -1) {
                            perror("Erreur fermeture du fichier de sortie");
                        }
                        exit(EXIT_FAILURE);
                    }
                    if(close(fd) == -1) {
                        perror("Erreur fermeture du fichier de sortie");
                    }
                }

                //Exécute la commande avec les arguments
                if (execvp(arguments[0], arguments) == -1) {
                    perror("Erreur lors de l'exécution de la commande");
                    exit(EXIT_FAILURE);
                }
            } else {
                // Processus père : Attendre la fin du processus fils
                int status;
                if (waitpid(pid, &status, 0) == -1) {
                    perror("Erreur lors de l'attente du processus fils");
                    exit(EXIT_FAILURE);
                }

                //Libérer la mémoire des arguments
                freeMemory(argc, arguments);
            }
        }
    }
    return 0;
}
