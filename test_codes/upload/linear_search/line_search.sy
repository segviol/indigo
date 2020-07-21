int MAX=1000;
int intArray[1000];

int find(int data){
   int comparisons;
   int index;
   int i;
   
   comparisons = 0;
	index = -1;
	
   i=0;
   while(i<MAX){
      // count the comparisons made 
      comparisons = comparisons + 1;
      // if data found, break the loop	
      if(data == intArray[i]){
         index = i;
         break;
      }
      i=i+1;
   }   
   putint(comparisons);
   putch(10);
   return index;
}

void display(){
   int i;
   i=0;
   while(i<MAX){
      putint(intArray[i]);
      i=i+1;
   }
   putch(10);
}

int main(){
	int i;
	int location;
	int QQQQ;
	
	QQQQ=getint();
	i=0;
	while (i<MAX) {
		intArray[i] = i;
		i=i+1;
	}
   display();
   location = find(QQQQ);
   putint(location);
   putch(10);
   return 0;
}
