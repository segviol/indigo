int arr[100];

void selectSort(int n)
{
	int i = 0;
	int j = 0;
	int minValue;
	int tmp;
	while (i < n - 1)
	{
		minValue = i;
		j = i + 1;
		while (j < n)
		{
			if (arr[minValue] > arr[j])
			{
				minValue = j;
			}
			j = j + 1;
		}
		if (minValue != i)
		{
			tmp = arr[i];
			arr[i] = arr[minValue];
			arr[minValue] = tmp;
		}
		i = i + 1;
	}
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
	selectSort(n);
	printArray(n);
	return 0;
}