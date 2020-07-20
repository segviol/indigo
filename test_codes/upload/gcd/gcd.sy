int gcd1(int m,int n)
{    
    int t,r;    
    if (m<n) 
    {        
        t=m;        
        m=n;       
        n=t;    
    }    
 
    while((m%n)!=0)  
    {        
        r=m%n;        
        m=n;        
        n=r;    
    }   
 
    return n;
}

int gcd2(int x, int y)
{	if (y!=0)			
            return gcd2(y, x%y);		
        else			
            return x;
} 
int main(){
  int num=getint();
  while(num>0){
	  int m=getint();
	  int n=getint();
	  int t1=gcd1(m,n);
    int t2=gcd2(m,n);
    putint(t1);
    putch(10);
    putint(t2);
    putch(10);
    num=num-1;
  }

	return 0;
}