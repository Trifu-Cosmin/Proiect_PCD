/*
  Fisier cu warning intentionat:
  variabila x este folosita fara initializare.
*/

#include <stdio.h>

int main(void)
{
    int x;
    int y = x + 5;

    printf("%d\n", y);

    return 0;
}
