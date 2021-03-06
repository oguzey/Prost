#ifndef PROST_APE_H
#define PROST_APE_H

typedef struct {
    __u8 *data;
    size_t size;
} Data;

Data *data_create(__u8 *a_data, size_t a_size);
void data_destroy(Data *data);
void data_print(Data *data, char *msg);

void prost_encrypt(Data *data, Data *nonce, Data *assoc_data, const char *pass,
                   Data **CT, Data ** tag);

int prost_decrypt(Data *CT, Data *tag, Data *nonce, Data *assoc_data,
                  const char *pass, Data **OT);
//void prost_encrypt(__u8 *data, size_t size
//                   , __u8 **ct, size_t *size_ct
//                   , __u8 **tag, size_t *size_tag
//                   ,__u8 *nonce, size_t nonce_size
//                   , __u8 *a_data, size_t a_data_size);

//int prost_decrypt(__u8 *ct, size_t ct_size
//                  , __u8 *tag, size_t tag_size
//                  , __u8 **output, size_t *size_output
//                  ,__u8 *nonce, size_t nonce_size
//                  , __u8 *a_data, size_t a_data_size);

#endif // PROST_APE_H
