#include<stdio.h>

int n,flag[15],a[15];
void dfs(int k,int num[]){
    if(k==n){   //递归结束条件，当到达第n层，代表第n-1层已经填完
        for(int i=0;i<n;i++){
            printf("%d ",a[i]);
        }
        printf("\n");
    }
    for(int i=0;i<n;i++){  //n个数字 
        if(flag[i]==0){  //当前数字没用过才可以排列
            a[k]=num[i];      //第k层（第k个位置）填数字num[i]
            flag[i]=1;   //该数已经用过，置为1
            dfs(k+1,num);    //填下一层（下一个位置）
            flag[i]=0;   //递归回来的时候需要重新置为0 
        }
    }
}
int main()
{
	scanf("%d",&n);
    int num[15]={};
    for(int i=0;i<n;i++)
    {
        scanf("%d",&num[i]);
    }
    int temp;
    for(int i=0;i<n;i++)
        for(int j=0;j<n-1;j++)
        {
            if(num[j]>num[j+1])
            {
                temp=num[j];
                num[j]=num[j+1];
                num[j+1]=temp;
            }
        }
	dfs(0,num);     //从第0层（第一个位置）开始排列 
	return 0;
}