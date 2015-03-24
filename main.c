#include <stdio.h>
#include <linux/types.h>
#include <assert.h>

#include "Prost_Permutation.h"

static int test_prost_permutation(void)
{
    __u8 input[64];
    __u8 output_perm[64];
    __u8 output_inv_perm[64];

    int i = 0;

    for(i = 0; i < 64; ++i) {
        input[i] = i;
        output_perm[i] = 0;
    }

    prost_permutation(input, 64, output_perm);
    prost_permutation_inverse(output_perm, 64, output_inv_perm);

    for(i = 0; i < 64; ++i) {
        if (input[i] != output_inv_perm[i]) {
            printf("Test permutation fail.\n");
            return 1;
        }
    }
    printf("Test permutation OK.\n");
    return 0;
}

int main(void)
{
    printf("Hello World!\n");
    assert(test_prost_permutation() == 0);
    return 0;
}

