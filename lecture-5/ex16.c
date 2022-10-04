#include <stdio.h>  // i/o functions
#include <assert.h> // assert function
#include <stdlib.h> // common lib functions such as malloc and free
#include <string.h> // string utils

// 20 byte struct (not including the contents of the string pointed to by name)
struct Person {
  char *name; // word size: 8 bytes on this 64-bit server
  int age;    // 4 bytes
  int height; // 4 bytes
  int weight; // 4 bytes
};

// Creates a new instance of Person and returns a pointer to it. The instance is
// allocated on the heap, so it won't be destroyed upon function return.
struct Person *Person_create(char *name, int age, int height, int weight) {
  struct Person *who = malloc(sizeof(struct Person));
  assert(who != NULL); // just a sanity check for learning purposes

  // who->name is syntactic sugar for (*who).name
  who->name = strdup(name); // copy the string contents pointed to
  who->age = age;
  who->height = height;
  who->weight = weight;

  return who;
}

// Deallocate the Person instance pointed to (and the string referenced from within it)
void Person_destroy(struct Person *who) {
  assert(who != NULL);
  free(who->name); // just calling free(who) would remove the pointer to name but not the contents of name)
  free(who);
}

void Person_print(struct Person *who) {
  printf("Name: %s\n", who->name);
  printf("\tAge: %d\n", who->age);
  printf("\tHeight: %d\n", who->height);
  printf("\tWeight: %d\n", who->weight);
}

int main(int argc, char *argv[]) {
  // doxxing myself
  struct Person *adrian = Person_create("Adrian Borup", 21, 183, 60);

  Person_print(adrian);
  printf("I seemingly live at %p, come find me there!\n", adrian);

  Person_destroy(adrian);

  // Output:
  // Name: Adrian Borup
  //         Age: 21
  //         Height: 183
  //         Weight: 60
  // I seemingly live at 0x55f66c7492a0, come find me there!

  // Failed assertion when given NULL:
  //   ex16.c:31: Person_destroy: Assertion `who != NULL' failed.
  //   Aborted (core dumped

  return 0;
}
