#include <stdio.h>
#include <linux/types.h>

#include "Prost_Permutation.h"

int main(void)
{
    printf("Hello World!\n");
    __u8 input[64];
    __u8 output[64];

    int i = 0;

    for(i = 0; i < 64; ++i) {
        input[i] = i;
        output[i] = 0;
    }

    check_mapping(input, 64, output);

    for(i = 0; i < 64; ++i) {
        printf("%02x ", input[i]);
    }
    printf("\n");
    for(i = 0; i < 64; ++i) {
        printf("%02x ", output[i]);
    }
    printf("\n");

    return 0;
}

