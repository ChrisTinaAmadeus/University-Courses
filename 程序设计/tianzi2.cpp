#include <stdio.h>
#include <string.h>

const int MAXSIZE = 2000;
const int dr[4] = {1, -1, 0, 0}, dc[4] = {0, 0, 1, -1};
int m, n; // m行n列
int len;
char str[MAXSIZE + 10];
char board[MAXSIZE][MAXSIZE];
struct save
{
    int r, c;
    int type;
    int cover;
};

bool inside(int x, int max)
{
    if (x <= max && x >= 0)
        return true;
    else
        return false;
}

int main()
{
    scanf("%d%d", &m, &n);
    // getchar();
    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
            scanf(" %c", &board[i][j]);
    scanf("%s", str);
    len = strlen(str);
    save savedata[MAXSIZE] = {};
    int savecnt = 0;

    for (int i = 0; i < m; i++)
        for (int j = 0; j < n; j++)
        {
            for (int type = 0; type < 4; type++)
            {
                int cover = 0;
                bool dec = false;
                if (inside(i + dr[type] * (len - 1), m - 1) && inside(j + dc[type] * (len - 1), n - 1))
                {
                    dec = true;
                    for (int l = 0; l < len; l++)
                    {
                        int target = board[i + dr[type] * l][j + dc[type] * l];
                        int object = str[l];
                        if (target != object && target != '0')
                        {
                            dec = false;
                            break;
                        }
                        if (target == object)
                            cover++;
                    }
                }
                int r1 = i - dr[type], c1 = j - dc[type];
                int r2 = i + dr[type] * len, c2 = j + dc[type] * len;
                if (inside(r1, m-1) && inside(c1, n-1) && board[r1][c1] != '1')
                    dec = false;
                if (inside(r2, m-1) && inside(c2, n-1) && board[r2][c2] != '1')
                    dec = false;
                if (dec)
                {
                    savedata[savecnt] = {i, j, type, cover};
                    savecnt++;
                    break;
                }
            }
        }
    for (int i = 0; i < savecnt - 1; i++)
        for (int j = 0; j < savecnt - i - 1; j++)
            if (savedata[j].cover < savedata[j + 1].cover)
            {
                save temp = savedata[j];
                savedata[j] = savedata[j + 1];
                savedata[j + 1] = temp;
            }
    if (savecnt > 0)
        printf("%d %d\n%d %d", savedata[0].r, savedata[0].c, savedata[0].r + dr[savedata[0].type] * (len - 1),
               savedata[0].c + dc[savedata[0].type] * (len - 1));
    else
        printf("No");
    return 0;
}