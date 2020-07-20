int N;
int a[100];
int out[100][10];
int cnt;

void NQueen(int k)
{
	int i;
	int j;
	if(k==N) {
		cnt = cnt+1;
		i = 0;
		while (i < N) {
			out[cnt][i]=a[i]+1;
			i = i + 1;
		} 
		return; 
	}
	i = 0;
	while(i<N){
		j = 0;
		while(j<k){
			if(a[j]==i||(a[j]-i)==(k-j)||(a[j]-i)==(j-k)){
				break;
			}	
			j = j+1;
		}
		if(j==k){
			a[k]=i; 
			NQueen(k+1);
		}
		i=i+1;
	}
}

int main()
{	
	int n;
	int num;
	int i;
	
	N=8;
	cnt = 0;
	NQueen(0);
	
	n = getint();
	while(n){
        n = n - 1;
		num = getint();
		i=0;
		while (i<8){
			putint(out[num][i]);
			i=i+1;
		} 
		putch(10);
	}
	return 0;
}
