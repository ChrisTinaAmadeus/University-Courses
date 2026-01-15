#include <stdio.h>
int main()
{
	char numstr[100], b[100], c[100];
	scanf("%s", numstr);
	scanf("%s %s", b, c);
	printf("%s\n", numstr);
	printf("%s %s", b, c);
	return 0;
}