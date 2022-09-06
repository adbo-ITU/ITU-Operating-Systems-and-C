#include <stdio.h>

int main(int argc, char *argv[]) {
    int integer = 123;
    float fractional = 32.3;
    double thicc_fractional = 1234542.4321412;

    char single_char = 'A';
    char string_1[] = "Frodo Baggins";
    char string_2[] = "Samwise Gamgee";

    printf("int: %d\n", integer);
    printf("float: %f\n", fractional);
    printf("double: %f\n", thicc_fractional);
    printf("char: %c\n", single_char);
    printf("string 1: %s\n", string_1);
    printf("string 2: %s\n", string_2);

    int plz = 420;
    double pls = 13.37;

    printf("int again..: %d\n", plz);
    printf("double again..: %f\n", pls);

    long thicc = 12341431L * 31242412512L;
    printf("long boi: %ld\n", thicc);

    printf("int.. or char.. or int.. ?? %c %d\n", 'A' * 123, 'A' * 123);

    // And now... drumroll... BOOM
    printf("string: %s, char: %c\n", '\0', '\0');
}
