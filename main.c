#include <stdio.h>
#include <linux/types.h>
#include <assert.h>
#include <stdlib.h>

#include "Prost_Permutation.h"
#include "Prost_APE.h"

static int test_prost_permutation(void)
{
    __u8 input[64];
    __u8 output_perm[64];
    __u8 output_inv_perm[64];

    int i = 0;

    for(i = 0; i < 64; ++i) {
        input[i] = i;
        output_perm[i] = 0;
        output_inv_perm[i] = 0;
    }

    prost_permutation(input, 64, output_perm, 64);
    prost_permutation_inverse(output_perm, 64, output_inv_perm, 64);

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

    __u8 input[64];
    int i = 0;

    for(i = 0; i < 64; ++i) {
        input[i] = i;
    }

    __u8 *ct = NULL;
    __u8 *tag = NULL;
    size_t size_ct = 0, size_tag = 0;
    prost_encrypt(input, 64, &ct, &size_ct, &tag, &size_tag);
    printf("Cypher text \n");
    for(i = size_ct - 1; i >= 0; --i) {
        printf("%02x ", ct[i]);
    }
    printf("\nTag \n");
    for(i = size_tag - 1; i >= 0; --i) {
        printf("%02x ", tag[i]);
    }
    printf("\n");

    __u8 *M = NULL;
    size_t size_M = 0;
    int res = prost_decrypt(ct, size_ct, tag, size_tag, &M, &size_M);

    printf("\nres dec is %d\n", res);
    printf("Open text \n");
    for(i = 0; i < (int)size_M; ++i) {
        printf("%02x ", M[i]);
    }
    printf("\n");
    free(M);
    free(ct);
    free(tag);
    return 0;
}

