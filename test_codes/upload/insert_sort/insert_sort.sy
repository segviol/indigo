int a[1000];

/* 插入排序 */
void InsertSort(int a[],int n)
{
	int p;
	int i;
	int tmp;
	
	p =1;
	while(p<n){
		tmp=a[p];/*取出未排序序列中的第一个元素*/
		i=p;
		while(i>=1&&a[i-1]>tmp){
			a[i]=a[i-1];/*依次与已排序序列中元素比较并右移*/
			i=i-1;
		}
		a[i]=tmp;/*放进合适的位置 */
		p=p+1;
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
	InsertSort(a,size);
	putarray(size,a); 
	putch(10);
	return 0;
}