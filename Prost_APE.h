#ifndef PROST_APE_H
#define PROST_APE_H

void prost_encrypt(__u8 *data, size_t size
                   , __u8 **ct, size_t *size_ct, __u8 **tag, size_t *size_tag);

int prost_decrypt(__u8 *ct, size_t ct_size
                  , __u8 *tag, size_t tag_size
                  , __u8 **output, size_t *size_output);

#endif // PROST_APE_H
