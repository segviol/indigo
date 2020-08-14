int dp[55][55][55][55];
int a[55][55];

int mmax(int a, int b) {
	if (a>b) {
		return a;
	}
	else {
		return b;
	}
}

int main()
{
    int m;
	int n;
	int i;
	int j;
	int k;
	int l;
	    
    m=getint();
    n=getint();
    
    i=1;
    while (i<=m) {
		j=1;
        while (j<=n) {
            a[i][j]=getint();
            j=j+1;
        }
        i=i+1;
    }
    
    i=1;
    while(i<=m){
    	j=1;
        while(j<=n){
        	k=1;
            while(k<=m){
            	l=1;
                while(l<=n){
                    dp[i][j][k][l]=
					mmax(mmax(dp[i-1][j][k-1][l],dp[i-1][j][k][l-1]),                          
						mmax(dp[i][j-1][k][l-1],dp[i][j-1][k-1][l]))+a[i][j]+a[k][l];
                    if (i==k && j==l) { 
                        dp[i][j][k][l] = dp[i][j][k][l] - a[k][l];
                    }
                    l=l+1;
                }
                k=k+1;
            }
            j=j+1;
        }
        i=i+1;
    }
    putint(dp[m][n][m][n]);
    return 0;
}