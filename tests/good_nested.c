/*
  Fisier valid pentru testarea if-urilor si a mai multor functii.
*/

#include <stdio.h>

int max_value(int a, int b)
{
    if (a > b)
    {
        return a;
    }

    return b;
}

int is_positive(int x)
{
    if (x > 0)
    {
        return 1;
    }

    return 0;
}

int main(void)
{
    int a = 10;
    int b = 20;
    int maximum = max_value(a, b);

    if (is_positive(maximum))
    {
        printf("%d\n", maximum);
    }

    return 0;
}
