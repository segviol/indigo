int arr[100];

void merge(int start, int mid, int end) {
	int result[100];
	int k = 0;
	int i = start;
	int j = mid + 1;
	while (i <= mid && j <= end) {
		if (arr[i] < arr[j]) {
			result[k] = arr[i];
			k = k + 1;
			i = i + 1;
		}
		else {
			result[k] = arr[j];
			k = k + 1;
			j = j + 1;
		}
	}
	if (i == mid + 1) {
		while (j <= end) {
			result[k] = arr[j];
			k = k + 1;
			j = j + 1;
		}
	}
	if (j == end + 1) {
		while (i <= mid) {
			result[k] = arr[i];
			k = k + 1;
			i = i + 1;
		}
	}
	j = 0;
	i = start;
	while(j < k){
		arr[i] = result[j];
		i = i + 1;
		j = j + 1;
	}
}

void mergeSort( int start, int end) {
	if (start >= end)
		return;
	int mid = (start + end) / 2;
	mergeSort(start, mid);
	mergeSort(mid + 1, end);
	merge(start, mid, end);
}

void printArray(int n)
{
    int i = 0;
    while (i < n)
    {
        putint(arr[i]);
        putch(32);
        // printf("%d ", arr[i]);
        i = i + 1;
    }
}

int main()
{
    int n;
    int i = 0;
    n = getint();
    while (i < n)
    {
        arr[i] = getint();
        i = i + 1;
    }
    mergeSort(0,n-1);
    printArray(n);
    return 0;
}