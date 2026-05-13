/*
  Fisier cu warning/eroare de tip intentionata:
  se incearca initializarea unui int cu un string.
*/

#include <stdio.h>

int main(void)
{
    int value = "text";

    printf("%d\n", value);

    return 0;
}
