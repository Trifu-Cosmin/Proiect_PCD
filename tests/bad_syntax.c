/*
  Fisier cu eroare de sintaxa intentionata:
  lipseste valoarea din expresia de return.
*/

#include <stdio.h>

int broken_function(void)
{
    return ;
}

int main(void)
{
    int value = broken_function();

    printf("%d\n", value);

    return 0;
}
