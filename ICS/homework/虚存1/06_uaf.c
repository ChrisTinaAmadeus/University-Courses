#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    int *data = (int *)malloc(100 * sizeof(int));
    if (data == NULL)
    {
        perror("malloc");
        return 1;
    }

    for (int i = 0; i < 100; i++)
    {
        data[i] = i * 2;
    }

    free(data);

    printf("data[10]=%d\n", data[10]);

    return 0;
}
