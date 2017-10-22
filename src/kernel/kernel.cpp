#include <stdio.h>


extern "C" int kernel_main() {
    
    printf("%c", 'A');
    printf("%c", 'B');
    printf("%c", 'C');
    printf("%c", 'D');
    printf("%c\n", 'E');

    printf("%s\n", "Hello, World");

    printf("X: %d\n", -12345);
    printf("X: %d\n", 45);

    printf("Y: %o\n", 1337);

    printf("Z: %x\n", 1234);
    printf("Z: %x\n", 716714982);

    printf("Z: %X\n", 198739812);
    printf("Z: %X\n", 878798);

    return 0;
}