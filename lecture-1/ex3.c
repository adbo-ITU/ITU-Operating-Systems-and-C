#include <stdio.h>

int main(int argc, char *argv[])
{
  // If not initialized, the following is printed:
  //   I am -1405059071 years old.
  int age = 10;
  int height = 72;

  // To crash the system and activate the debugger, uncomment the following:
  // char *lulw = NULL;
  // printf("%c", lulw[123]);

  // If age is removed, the following is printed:
  //   I am 67600152 years old.
  printf("I am %d years old.\n", age);
  printf("I am %d inches tall.\n", height);

  return 0;
}
