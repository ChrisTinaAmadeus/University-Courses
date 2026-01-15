#include<stdio.h>
int main()
{
    int A,B,C,D,E,F,G,H,n,m=0;
    int a=0,b=0,c=0,d=0,e=0,f=0,g=0,h=0;
    scanf("%d",&n);
    for(A=0;A<=1;A++){
        for(B=0;B<=1;B++){
            for(C=0;C<=1;C++){
                for(D=0;D<=1;D++){
                    for(E=0;E<=1;E++){
                        for(F=0;F<=1;F++){
                            for(G=0;G<=1;G++){
                                for(H=0;H<=1;H++)
                                {
                                    int c1=H||F;
                                    int c2=B;
                                    int c3=G;
                                    int c4=!B;
                                    int c5=!(H||F);
                                    int c6=!F&&!H;
                                    int c7=!C;
                                    int c8=H||F;
                                    if(c1+c2+c3+c4+c5+c6+c7+c8==n)
                                    {
                                        if(A+B+C+D+E+F+G+H==1)
                                        {
                                        if(A==1)a=1;
                                        else if(B==1)b=1;
                                        else if(C==1)c=1;
                                        else if(D==1)d=1;
                                        else if(E==1)e=1;
                                        else if(F==1)f=1;
                                        else if(G==1)g=1;
                                        else if(H==1)h=1;
                                        m=1;
                                    
                                        }
                                        
                                    }
                                
                                }
                            
                            }
                        }
                    }
                }
            }
        }
    } 
if(a+b+c+d+e+f+g+h==1&&m==1)
{
    if(a==1)printf("A");
    else if(b==1)printf("B");
    else if(c==1)printf("C");
    else if(d==1)printf("D");
    else if(e==1)printf("E");
    else if(f==1)printf("F");
    else if(g==1)printf("G");
    else if(h==1)printf("H");
}   
else if(a+b+c+d+e+f+g+h!=1||m!=1)
printf("DONTKNOW");
return 0;
}