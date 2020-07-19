int n;
int k;
int cnt;
int a[60];

void solve1(int p,int sum,int last)
{
	int i;
	if(sum==n){
		if(p==k){
			cnt = cnt+1;
		}
		return;
	}
	if(sum>n){
		return;
	}
	i=last;
	while(i>=1){
		a[p]=i;
		sum=sum+i;
		solve1(p+1,sum,i);
		sum=sum-i;
		i=i-1;
	}
}

void solve2(int p,int sum,int last)
{
	int i;
	if(sum==n){
		cnt = cnt+1;
		return;
	}
	if(sum>n){
		return;
	}
	i=last;
	while(i>=1){
		a[p]=i;
		sum=sum+i;
		solve2(p+1,sum,i-1);
		sum=sum-i;
		i=i-1;
	}
}

void solve3(int p,int sum,int last)
{
	int i;
	if(sum==n){
		cnt = cnt+1;
		return;
	}
	if(sum>n){
		return;
	}
	i=last;
	while(i>=1){
		a[p]=i;
		sum=sum+i;
		solve3(p+1,sum,i);
		sum=sum-i;
		i=i-2;
	}
}

int main()
{
		n = getint();
		k = getint();
		cnt=0;
		solve1(0,0,n);
		putint(cnt);
		putch(10);
		cnt=0;
		solve2(0,0,n);
		putint(cnt);
		putch(10);
		cnt=0;
		if(n%2==0) {
			solve3(0,0,n-1);
		}
		else {
			solve3(0,0,n);
		}
		putint(cnt);
		putch(10);
	
	return 0;
} 
