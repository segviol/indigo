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

int solve(int l, int r) {  //记忆化递归 
	int max1;
	int max2;
	int k;
	int tmp;
	
    if (dp[l][r]) {
        return dp[l][r];
    }
    if (l == r) {
        dp[l][r] = a[l];
        root[l][r] = l;
    }
    max1 = solve(l+1,r) + solve(l,l); //l为根
    if (max1 > dp[l][r]) {
        dp[l][r] = max1;
        root[l][r] = l;
    }
    max2 = solve(l,r-1) + solve(r,r); //r为根
    if (max2 > dp[l][r]) {
        dp[l][r] = max2;
        root[l][r] = r;
    }
	k=l+1;
    while (k<r) {
        tmp = solve(l,k-1) * solve(k+1,r) + solve(k,k);  //k为根
        if (tmp > dp[l][r]) {
            dp[l][r] = tmp;
            root[l][r] = k;  //记录根
        }
        k=k+1;
    }
    return dp[l][r];
}

int main() {
    int i;
	
    n = getint();
    
    i=1;
    while (i<=n) {
        a[i] = getint();
        dp[i][i] = a[i];  //分数
        root[i][i] = i; //根节点
        i=i+1;
    }
    putint(solve(1,n));
    output(1,n);  //输出前序遍历
    return 0;
}