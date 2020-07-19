int a[1000];
 
/* 希尔排序 - 用Sedgewick增量序列 */
void ShellSort(int a[],int n)
{
	/* 这里只列出一小部分增量 */
	int Sedgewick[10];
	int si;
	int D;
	int i;
	int p;
	int tmp;
	
	Sedgewick[0] = 929;
	Sedgewick[1] = 505;
	Sedgewick[2] = 209;
	Sedgewick[3] = 109;
	Sedgewick[4] = 41;
	Sedgewick[5] = 19;
	Sedgewick[6] = 5;
	Sedgewick[7] = 1;
	Sedgewick[8] = 0;
	
	si=0;
	while(Sedgewick[si]>=n) {
		si = si+1; 
	}
	
	D=Sedgewick[si];
	while(D>0){
		p=D;
		while(p<n){ /* 插入排序*/
			tmp=a[p];
			i=p;
			while(i>=D&&a[i-D]>tmp){
				a[i]=a[i-D];
				i=i-D;
			}
			a[i]=tmp;
			p=p+1;
		}
		si=si+1;
		D=Sedgewick[si];
	}
} 

int main()
{
	int size;
	int i;
	size = getint();
	i = 0;
	while(i<size){
		a[i] = getint();
		i = i+1;
	} 
	ShellSort(a,size);
	putarray(size,a); 
	putch(10);
	return 0;
}