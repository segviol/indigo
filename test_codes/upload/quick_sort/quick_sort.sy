int arr[100];

int partition(int l, int r)
{
	int i;
	int j;
	int tmp;
	i = l + 1;
	j = l;

	while (i <= r)
	{
		if (arr[i] > arr[l])
		{
			i = i + 1;
		}
		else
		{
			tmp = arr[j + 1];
			arr[j + 1] = arr[i];
			arr[i] = tmp;
			i = i + 1;
			j = j + 1;
		}
	}
	tmp = arr[j];
	arr[j] = arr[l];
	arr[l] = tmp;
	return j;
}

void _quickSort(int l, int r)
{
	int key;
	if (l >= r)
	{
		return;
	}
	key = partition( l, r);
	_quickSort( l, key - 1);
	_quickSort( key + 1, r);
}

void quickSort(int n)
{
	_quickSort(0, n - 1);
	return;
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
    quickSort(n);
    printArray(n);
    return 0;
}