const int MAXNUM = 128 ;

int factorial(int n){
   if(n<=1) return (1);
   return(n*factorial(n-1)) ;
}

int mod(int x, int y){
   x=x-x/y*y;

   return (x) ;
}

void swap(int x, int y){
 int temp;

 putint(x);
 putch(32);
 putint(y);
 putch(10);
 temp = x;
 x=y;
 y=temp;
 putint(x);
 putch(32);
 putint(y);
 putch(10);
}

int fullnum(int n, int j, int a){
  return (n*100+j*10+a) ;
}

int flowernum(int n, int j, int a){
  return (n*n*n+j*j*j+a*a*a) ;
}

void completeflowernum()
{
  int k[128];
  int i,j,n,s,x1,y;
  int m,k2,h,leap,x2;
  int a,b,c ;

  j = 2;
  while (j < MAXNUM) {
    n = -1;
    s = j;
    i = 1;
    while(i<j) {
      x1 = (j/i) * i ;
      if( mod(j,i) == 0 ) {
        n = n + 1;
        s = s - i;
        if (n >= 128)
          putint(-1);
        else
          k[n] = i;
      }
      i = i + 1;
    }
    if(s==0) {
      putint(j);
      putch(10);
      i = 0;
      while(i <= n) {
        putint(k[i]);
        putch(10);
        i = i + 1;
      }
    }
  j = j + 1;
  }
  
  y = 0 ;
  i = 100;
  while (i < 100+MAXNUM) {
    n=i/100;
    j=mod(i/10,10);
    a=mod(i,10);
    if(fullnum(n,j,a)==flowernum(n, j, a)){
      k[y] = i ;
      y = y + 1 ;
    }
    i = i + 1;
  }
  i = 0;
  while(i<y){
    putint(k[i]);
    putch(10);
    i = i + 1;
  }
  
  h = 0 ;
  leap = 1 ;
  m = 2;
  while(m <= MAXNUM)
  {
    k2 = m / 2;
    i = 2;
    while(i <= k2) {
      x2 = (m/i)*i ;
      if( mod(m,i) == 0)
      {
        leap=0;
      }
      i = i + 1;
     }
    if(leap == 1)
    {
      putint(m);
      putch(10);
      h = h +1;

      x2 = (h/10)*10 ;

     }
     leap=1;
     m = m + 1;
  }

  putint(h);
  putch(10);
} 

int main()
{
   int n ,m,t;
   m = getint();
   t = getint();
   n = factorial(m) ;
   putint(n);
   putch(10);
   swap(t, 2*t ) ;
   completeflowernum() ;
   return 0;
}