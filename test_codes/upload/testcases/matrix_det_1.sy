const int N = 205;

// gets matrix values from user
void get_matrix (int M[][N],int m, int n)
{
	int i;
	int j;
	i=0;
	while (i<m)
	{
		j=0;
		while (j<n) {
			M[i][j] = getint();
			j=j+1;
		}
		i=i+1;
	}
}
 
// prints matrix of mXn
void print_matrix (int M[][N],int m, int n)
{
	int i;
	int j;
	int temp;
	i=0;
	while (i<m) //go over columns
	{
		j=0;
		while (j<n)  //go over rows
		{
			putint(M[i][j]);
			putch(32);
			j=j+1;
		}
		putch(10);
		i=i+1;
	}
}


int powfff(int n, int times) {
	int i;
	int res;
	
	if (times == 0) {
		return 1;
	}
	i=0;
	res = n;
	while(i<times-1){
		res = res*n;
		i=i+1;
	}	
	return res;
}

void GetSubmatrix(int M[][N],int submatrix[][N],int n,int neglect_line)
{
    int i;
	int j;
	int k;
	
	i=0;
    while(i<n-1)
    {
    	j=0;
    	k=0;
        while(j<n-1)
        {
            if(k!=neglect_line)
            {
                submatrix[i][j]=M[i+1][k];
                j=j+1;
            }
            k=k+1;
        }
        i=i+1;
    }
}


int CalculateMatrix(int M[][N],int submatrix[][N],int m, int n)
{
    int i;
    int sum;
    int submatrix_value;
    sum = 0;
    if(n==2)
    {
        return M[0][0]*M[1][1]-M[1][0]*M[0][1];
    }
    else
    {
    	i = 0;
        while(i<n)
        {
            GetSubmatrix(M,submatrix,n,i);
            submatrix_value=CalculateMatrix(submatrix,submatrix, n-1, n-1);
            sum=sum + powfff(-1,i) * M[0][i] * submatrix_value;
            i=i+1;
        }
        return sum;
    }

}

int main()
{
	int n;
	int m;
	int result;
	
	int M[N][N];
	int submatrix[N][N];
	
	m=getint();
	n=getint();

    get_matrix (M,m,n); //get matrix from user
    
    print_matrix(M,m,n); //print matrix
    
    result = CalculateMatrix(M, submatrix, m,n);
    
    putint(result);
    return 0;
}