#ifndef PROST_APE_H
#define PROST_APE_H

void prost_encrypt(__u8 *data, size_t size
                   , __u8 **ct, size_t *size_ct, __u8 **tag, size_t *size_tag);

#endif // PROST_APE_H
