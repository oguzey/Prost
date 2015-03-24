#ifndef PROST_PERMUTATION_H
#define PROST_PERMUTATION_H
#include <linux/types.h>

void prost_permutation(__u8 *input, size_t size, __u8 *output);

void prost_permutation_inverse(__u8 *input, size_t size, __u8 *output);

#endif // PROST_PERMUTATION_H
