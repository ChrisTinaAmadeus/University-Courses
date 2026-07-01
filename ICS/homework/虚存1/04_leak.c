#include <stdio.h>
#include <stdlib.h>

int main(void)
{
    size_t n = 1024;
    char *buf = (char *)malloc(n);
    if (buf == NULL)
    {
        perror("malloc");
        return 1;
    }

    buf[0] = 'A';
    buf[n - 1] = 'Z';
    printf("buf[0]=%c, buf[%zu]=%c\n", buf[0], n - 1, buf[n - 1]);

    return 0;
}
