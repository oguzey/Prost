#ifndef PROST_PERMUTATION_H
#define PROST_PERMUTATION_H
#include <linux/types.h>

void prost_permutation(__u8 *input, size_t size_in
                       , __u8 *output, size_t size_out);

void prost_permutation_inverse(__u8 *input, size_t size_in
                               , __u8 *output, size_t size_out);

#endif // PROST_PERMUTATION_H
