int arr[100];

void bubbleSort(int n)
{
    int i = 0;
    int j = 0;
    int tmp;

    while (i < n - 1)
    {
        j = 1;
        while (j < n)
        {
            if (arr[j] < arr[j - 1])
            {
                tmp = arr[j];
                arr[j] = arr[j - 1];
                arr[j - 1] = tmp;
            }
            j = j + 1;
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
    bubbleSort(n);
    printArray(n);
    return 0;
}