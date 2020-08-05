
int A[1024][1024];
int B[1024];
int C[1024];

void mv(int n, int A[][1024], int B[], int C[])
{
    int i, k;

    i = 0;
    while (i < n)
    {
        C[i] = 0;
        i = i + 1;
    }

    i = 0;
    k = 0;
    while (k < n)
    {
        i = 0;
        while (i < n) //L8
        {
            C[i] = C[i] + A[k][i] * B[i];
            i = i + 1;
        }
        k = k + 1;
    }
}

int main()
{
    int n = getint();
    int i, j;

    i = 0;
    j = 0;
    while (i < n)
    {
        j = 0;
        while (j < n)
        {
            A[i][j] = getint();
            j = j + 1;
        }
        i = i + 1;
    }
    i = 0;
    j = 0;
    while (i < n)
    {
        B[i] = getint();
        i = i + 1;
    }

    starttime();

    i = 0;
    while (i < 500)
    {
        mv(n, A, B, C);
        mv(n, A, C, B);
        i = i + 1;
    }

    int ans = 0;
    i = 0;
    while (i < n)
    {
        j = 0;
        while (j < n)
        {
            ans = ans + B[i];
            j = j + 1;
        }
        i = i + 1;
    }
    stoptime();
    putint(ans);
    putch(10);

    return 0;
}
