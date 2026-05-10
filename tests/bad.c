/*
  Descriere:
  Acest fisier este folosit pentru testarea diagnosticelor libclang.
  Codul este aproape valid, dar contine o variabila folosita neinitializata,
  astfel incat libclang poate afisa warning-uri.
 */

#include <stdio.h>

int main(void)
{
    int x;
    printf("%d\n", x);
    return 0;
}

