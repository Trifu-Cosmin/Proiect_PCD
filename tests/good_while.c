/*
  Fisier valid pentru testarea buclelor while.
*/

#include <stdio.h>

int count_digits(int number)
{
    int count = 0;

    while (number > 0)
    {
        number = number / 10;
        count++;
    }

    return count;
}

int main(void)
{
    int value = 12345;
    int digits = count_digits(value);

    printf("%d\n", digits);

    return 0;
}
