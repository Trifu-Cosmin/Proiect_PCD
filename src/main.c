/*
  Descriere:
  Acest fisier este pastrat pentru compatibilitate cu varianta initiala
  a proiectului, in care executabilul principal se numea main.

  In implementarea curenta, logica de analiza se afla in analyzer.c.
  Acest program doar lanseaza executabilul analyzer folosind execv si transmite
  mai departe aceleasi argumente primite din linia de comanda.

  Astfel, ambele comenzi sunt valide:
    ./main -f tests/sample.c
    ./analyzer -f tests/sample.c
*/

#include <stdio.h>  // Pentru perror
#include <stdlib.h> // Pentru calloc, free
#include <unistd.h> // Pentru execv

int main(int argc, char *argv[])
{
    /*
      Construim un vector nou de argumente pentru analyzer.
      Primul argument trebuie sa fie numele programului executat.
    */
    char **new_argv = (char **)calloc((size_t)argc + 1U, sizeof(char *));
    if (new_argv == NULL)
    {
        perror("calloc");
        return 1;
    }

    new_argv[0] = "./analyzer";

    for (int i = 1; i < argc; i++)
    {
        new_argv[i] = argv[i];
    }

    new_argv[argc] = NULL;

    /*
      Inlocuim procesul curent cu analyzer.
      Daca execv reuseste, codul de dupa execv nu mai este executat.
    */
    execv("./analyzer", new_argv);

    /*
      Daca ajungem aici, inseamna ca execv a esuat.
    */
    perror("execv");
    free(new_argv);

    return 1;
}