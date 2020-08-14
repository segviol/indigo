int a[35];
int n;
int dp[35][35]; 
int root[35][35]; 

void output(int left, int right) {
	int r;
	
    if (left>right) {
        return;
    }
    r = root[left][right]; 
    putint(r);
    putch(32);
    output(left, r-1);  //递归左子树
    output(r+1, right);  //递归右子树
}

int main() {
	int i;
	int j;
	int k;
	int len;
	
    n = getint();
    
	i=1;
    while (i<=n) {
        a[i] = getint();
        dp[i][i] = a[i];  //分数
        root[i][i] = i; //根节点
        i=i+1;
    }
    
	len=2;
    while (len<=n) {  //遍历区间长度
		i=1; 
        while (i+len-1<=n) {  //遍历起点
            j = i+len-1;  //终点
            if (dp[i+1][j] + dp[i][i] > dp[i][j]) {
                dp[i][j] = dp[i+1][j] + dp[i][i];
                root[i][j] = i;
            }
            if (dp[i][j-1] + dp[j][j] > dp[i][j]) {
                dp[i][j] = dp[i][j-1] + dp[j][j];
                root[i][j] = j;
            }
			k=i+1;
            while (k<j) {
                if ((dp[i][k-1] * dp[k+1][j] + dp[k][k]) > dp[i][j]) {
                    dp[i][j] = dp[i][k-1] * dp[k+1][j] + dp[k][k];
                    root[i][j] = k;  //记录根
                }
                k=k+1;
            }
            i=i+1;
        }
        len=len+1;
    }
    putint(dp[1][n]);
    putch(10);
    output(1,n);  //输出前序遍历
    return 0;
}