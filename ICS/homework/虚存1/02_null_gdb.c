#include <stdio.h>

int main(void)
{
    int *p = NULL;
    printf("p=%p\n", (void *)p);
    *p = 42;
    printf("unreachable\n");
    return 0;
}
