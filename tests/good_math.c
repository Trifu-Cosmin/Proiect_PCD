/*
  Fisier valid pentru testarea functiilor si a buclelor.
*/

#include <stdio.h>

int square(int x)
{
    return x * x;
}

int sum_squares(int n)
{
    int sum = 0;

    for (int i = 1; i <= n; i++)
    {
        sum += square(i);
    }

    return sum;
}

int main(void)
{
    int n = 5;
    int result = sum_squares(n);

    if (result > 0)
    {
        printf("%d\n", result);
    }

    return 0;
}
