#include <stdio.h> 
int main() 
{
    for (int A = 0; A <= 1; A++) 
        for (int B = 0; B <= 1; B++) 
            for (int C = 0; C <= 1; C++) 
                for (int D = 0; D <= 1; D++) 
                     for (int E = 0; E <= 1; E++) 
                        for (int F = 0; F <= 1; F++)
                        {
                            int c1 = A || B;
                            int c2 = !(A && D);
                            int c3 = (A && E) || (A && F) || (E && F);
                            int c4 = (B && C) || (!B && !C);
                            int c5 = (C && !D) || (!C && D);
                            int c6 = D || (!D && !E);
                            if (c1 && c2 && c3 && c4 && c5 && c6)
                            {
                                printf("%d %d %d %d %d %d\n", A, B, C, D, E, F);
                                //break;
                            }
                            }
return 0;
}