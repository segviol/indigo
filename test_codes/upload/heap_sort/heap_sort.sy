int a[1000];

void PercDown(int a[],int p,int n)
{/* 下滤：将H中以H->Data[p]为根的子堆调整为最大堆 */
	int X;
	int Parent,Child;//下标从0开始，左儿子是Parent*2+1，n是取不到的位置 
	
	X=a[p]; /* 取出根结点存放的值 */
	
	Parent=p;
	while((Parent*2+1)<n){
		Child=Parent*2+1;
		if(Child!=n-1 && a[Child+1]>a[Child]){
			Child = Child+1;
		}
		if(X>=a[Child]){
			break;
		}
		else{
			a[Parent]=a[Child];
		}
		Parent=Child;
	}
	a[Parent]=X;	
}

/*堆排序*/
void HeapSort(int a[],int n)
{
	int i;
	int ttt;
	
	i=n/2-1;
	while(i>=0){/* 建立最大堆 */
		PercDown(a,i,n);
		i=i-1;
	}
	i=n-1;
	while(i>0){
		/* 删除最大堆顶 */
		ttt = a[0];
		a[0] = a[i];
		a[i] = ttt;
		PercDown(a,0,i);//把0位根节点调成最大堆,堆(删除最大元素后)的大小是i，i是取不到的位置 
		i=i-1;
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
	HeapSort(a,size);
	putarray(size,a); 
	putch(10);
	return 0;
}