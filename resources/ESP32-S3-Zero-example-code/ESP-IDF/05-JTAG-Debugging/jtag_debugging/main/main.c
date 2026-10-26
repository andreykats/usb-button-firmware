#include <stdio.h>

// Function to calculate the sum of all integers from 1 to 'number'
int summation(int number)
{
    int sum = 0;
    for (int i = 1; i <= number; i++){
        sum += i;
    }
    return sum;
}

void app_main(void)
{
    printf("Hello world!\n");
    int final_number = 6;
    int sum = summation(final_number);
    printf("The summation up to %d is %d\n", final_number, sum);
}