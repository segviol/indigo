
int BigIntarr[2][100];
int BigIntLength[2];
int Result[200];

int min(int a, int b) {
	if (a > b) {
		return b;
	}
	else {
		return a;
	}
}

int add() {
	int a[100];//加数
	int b[100];//被加数
	int c[100] = { 0 };//和
	int i, j = 0, k;//i用来循环，j表示进位 ，两加数中较小加数的长度放到k里
	int l1, l2, d;
	l1 = BigIntLength[0];
	l2 = BigIntLength[1];
	k = min(l1, l2);
	i = 0;
	while (i < BigIntLength[0]) {
		a[i] = BigIntarr[0][l1 - 1 - i];
		i = i + 1;
	}
	i = 0;
	while (i < BigIntLength[1]) {
		b[i] = BigIntarr[1][l2 - 1 - i];
		i = i + 1;
	}
	i = 0;
	while(i<k){
		d = a[i] + b[i] + j;
		j = d / 10;
		c[i] = d % 10;
		i = i + 1;
	}
	while (i < l1) {
		d = a[i] + j;
		j = d / 10;
		c[i] = d % 10;
		i = i + 1;
	}
	while (i < l2) {
		d = b[i] + j;
		j = d / 10;
		c[i] = d % 10;
		i = i + 1;
	}
	if (j > 0) { c[i] = j; i=i+1; }
	j = 0;
	k = i - 1;
	while (k > -1) {
		Result[j] = c[k];
		k = k - 1;
		j = j + 1;
	}	
	return j;
}

int main()
{
	int n;
	int i = 0;
    BigIntLength[0] = getarray(BigIntarr[0]);
    BigIntLength[1] = getarray(BigIntarr[1]);
	n = add();
    i = 0;
    while (i < n) {
        putint(Result[i]);
        i = i + 1;
	}
	return 0;
}