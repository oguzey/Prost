#include <stdio.h>
#include <linux/types.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "Prost_Permutation.h"
#include "Prost_APE.h"

#define panic(msg, ...) \
    printf(msg, ##__VA_ARGS__); \
    exit(1);

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

typedef enum {
    Encrypt = 0,
    Decrypt
} Action;

static char *_s_data_file = NULL;
static char *_s_enc_file = NULL;
static char *_s_mac_file = NULL;
static char *_s_password = NULL;
static char *_s_nonce_file = NULL;
static Action _s_action = -1;


static void get_args(int argc, char **argv)
{
    char *info;
    info = "Usage: Prost -a <action> -d <OT-file> -e <CT-file> "
            "-m <MAC-file> -p <password> [-n] <nonce-file>";

    int i;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i],"-d")==0 || strcmp(argv[i],"--data")==0) {
            if (argc > i + 1) {
                _s_data_file = argv[i + 1];
                i++;
            } else {
                panic("Missing argument for option %s\n", argv[i]);
            }
        } else if (strcmp(argv[i],"-e")==0 || strcmp(argv[i],"--enc-file")==0) {
            if (argc > i + 1) {
                _s_enc_file = argv[i + 1];
                i++;
            } else {
                panic("Missing argument for option %s\n", argv[i]);
            }
        } else if (strcmp(argv[i],"-m")==0 || strcmp(argv[i],"--mac-file")==0) {
            if (argc > i + 1) {
                _s_mac_file = argv[i + 1];
                i++;
            } else {
                panic("Missing argument for option %s\n", argv[i]);
            }
        } else if (strcmp(argv[i],"-p")==0 || strcmp(argv[i],"--password")==0) {
            if (argc > i + 1) {
                _s_password = argv[i + 1];
                i++;
            } else {
                panic("Missing argument for option %s\n", argv[i]);
            }
        } else if (strcmp(argv[i],"-n")==0 || strcmp(argv[i],"--nonce")==0) {
            if (argc > i + 1) {
                _s_nonce_file = argv[i + 1];
                i++;
            } else {
                panic("Missing argument for option %s\n", argv[i]);
            }
        } else if (strcmp(argv[i],"-a")==0 || strcmp(argv[i],"--action")==0) {
            if (argc > i + 1) {
                if (strcmp(argv[i + 1],"encrypt") == 0) {
                    _s_action = Encrypt;
                } else if (strcmp(argv[i + 1],"decrypt") == 0) {
                    _s_action = Decrypt;
                } else {
                    panic("Incorrect value for option %s. "
                          "Possible values are 'encrypt' and 'decrypt'\n", argv[i]);
                }
                i++;
            } else {
                panic("Missing argument for option %s\n", argv[i]);
            }
        } else {
            panic("%s\n", info);
        }
    }
    if (!_s_data_file || !_s_enc_file || !_s_mac_file
                || !_s_password || !_s_nonce_file) {
        panic("%s\n", info);
    }
}

static Data* read_data(const char *filename)
{
    struct stat sb;
    FILE *file = NULL;
    __u8 *data = NULL;
    int c;
    int i;
    size_t size = 0;

    if (stat(filename, &sb) == -1) {
        printf("Could not process file '%s'\n", filename);
        perror("Error was");
        exit(1);
    }
    size = sb.st_size;
    printf("The file '%s' has size: %d bytes\n", filename, (int)size);

    if ((file = fopen(filename, "r")) == NULL) {
        printf("Could not open file '%s'\n", filename);
        perror("Error was");
        exit(1);
    }
    data = malloc(size);
    if (data == NULL) {
        perror("Error was");
        exit(1);
    }
    memset(data, 0, size);
    i = 0;
    while ((c = fgetc(file)) != EOF) {
        data[i++] = c;
    }
    fclose(file);
    return data_create(data, size);
}

void write_data(Data *data, const char *filename)
{
    FILE *file;
    if ((file = fopen(filename, "w")) == NULL) {
        printf("Could not open file '%s'\n", filename);
        perror("Error was");
        exit(1);
    }

    int i = 0;
    for (;i < (int)data->size; ++i) {
        fputc(data->data[i], file);
    }
    fclose(file);
}

int main(int argc, char **argv)
{
    printf("Hello World!\n");
    assert(test_prost_permutation() == 0);

    get_args(argc, argv);

    Data *nonce = read_data(_s_nonce_file);
    data_print(nonce, "Nonce: ");
    Data *assoc_data = NULL;
    Data *OT = NULL;
    Data *CT = NULL;
    Data *mac = NULL;


    switch (_s_action) {
    case Encrypt:
        OT = read_data(_s_data_file);
        data_print(OT, "OT: ");
        prost_encrypt(OT, nonce, assoc_data, _s_password, &CT, &mac);
        write_data(CT, _s_enc_file);
        write_data(mac, _s_mac_file);
    break;
    case Decrypt:
        CT = read_data(_s_enc_file);
        mac = read_data(_s_mac_file);
        data_print(CT, "CT: ");
        data_print(mac, "mac: ");
        if (prost_decrypt(CT, mac, nonce, assoc_data, _s_password, &OT)) {
            printf("Error was occured in decryption.\n");
        }
        data_print(OT, "Got OT:");
        write_data(OT, _s_data_file);
    break;
    default:
        panic("Wrong action\n");
    break;
    }
    data_destroy(nonce);
    data_destroy(assoc_data);
    data_destroy(OT);
    data_destroy(CT);
    data_destroy(mac);
    return 0;
}

