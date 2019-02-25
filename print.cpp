/* strtok example */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
int a = 0;

int main() {
  char str[] = "Start! Available Hops #4";
  char * pch;
  printf("Splitting string \"%s\" into tokens:\n", str);
  pch = strtok(str, "#");
  int b = 1;

  while (pch != NULL) {
    printf("%s\n", pch);
    pch = strtok(NULL, " ");
  }
  printf("%d\n", a);
  srand(time(NULL));
  int v = rand() % 100;
  printf("%d\n", v);
  return 0;
}
