int isPrime[100005];
int prime[10005];

int main()
{
	int n;
	int q;
	int k;
	int cnt;
	int i;
	int j;
	
	n = getint();
	q = getint();
	
	cnt = 1;
	i=2;
	while (i<=n) {
		if (isPrime[i] == 0) {
			prime[cnt] = i;
			cnt = cnt+1;
		} 
		j=1;
		while (j<cnt && prime[j]*i<=n) {
			isPrime[prime[j]*i] = 1;
			if (i%prime[j] == 0) {
				break;
			}
			j=j+1;
		}
		i=i+1;	
	}
	i=0;
	while (i<q) {
		k=getint();
		putint(prime[k]);
		i=i+1;
	}
    return 0;
}