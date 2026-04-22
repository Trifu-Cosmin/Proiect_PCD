/*
  Descriere:
  Acest fisier reprezinta un exemplu valid de cod sursa folosit pentru testarea
  aplicatiei. El poate fi analizat fara erori si este util pentru demonstratia
  cazului in care parsarea reuseste corect.
 */

#include <stdio.h>

int add(int a, int b)
{
    return a + b;
}

int sum_until(int n)
{
    int sum = 0;

    for (int i = 0; i <= n; i++)
    {
        sum += i;
    }

    return sum;
}

int main(void)
{
    int x = 2;
    int y = 3;
    int result = add(x, y);

    if (result > 0)
    {
        printf("%d\n", sum_until(result));
    }

    return 0;
}