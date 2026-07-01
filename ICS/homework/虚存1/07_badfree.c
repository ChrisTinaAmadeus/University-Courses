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
        data[i] = i;
    }

    free(data + 50);

    printf("unreachable?\n");
    return 0;
}
