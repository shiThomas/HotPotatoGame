/* strtok example */
#include <stdio.h>
#include <string.h>
int a = 0;

int add() {
  a = 1;
  return 1;
}
int main() {
  char str[] = "Start! Available Hops #4";
  char * pch;
  printf("Splitting string \"%s\" into tokens:\n", str);
  pch = strtok(str, "#");
  int b = 1;
  add();
  while (pch != NULL) {
    printf("%s\n", pch);
    pch = strtok(NULL, " ");
  }
  printf("%d\n", a);
  return 0;
}
