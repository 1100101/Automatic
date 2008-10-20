
#include <stdint.h>

char *base64_encode(const char *input, uint32_t len, uint32_t * setme_len);
char *base64_decode(const char *encoded_string, uint32_t len, uint32_t * setme_len);
