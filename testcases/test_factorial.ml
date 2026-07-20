int n;
int factorial;

n = 5;
factorial = 1;

while (n > 1)
{
    factorial = factorial * n;
    n = n - 1;
}

print(factorial);
