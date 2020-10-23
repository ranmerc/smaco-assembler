#include <stdio.h>
#include <string.h>
void printer(char *arr[], int num)
{
  int i;
  for (i = 0; i < num; i++)
    printf("%s ", arr[i]);
}
int main(void)
{
  int a[4] = {0};
  printf("%d", a[3]);
}

// argc fix
// obj code declarative fix
// READ/PRINT 0 in register fix
// was adding 0 before 10 in print statement
// added value attribute to symbols