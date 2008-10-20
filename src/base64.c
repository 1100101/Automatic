/* $Id$
 * $Name$
 * $ProjectName$
 */

/**
 * @file base64.c
 *
 * Provides functions for Base64 encoding and decoding
 *
 * \internal Created on: Oct 12, 2008
 * \internal Author: Frank Aurich (1100101+automatic@gmail.com)
 */

#include <ctype.h>
#include <stdint.h>

#include "output.h"
#include "utils.h"

static const char alphabet[64] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";


/** \brief Encode input string to Base64.
 *
 * \param[in] input unencoded input data
 * \param[in] len size of input data
 * \param[out] setme_len size of the encoded data
 * \return pointer to b64-encoded string
 */
char *base64_encode(const char *input, uint32_t len, uint32_t * setme_len) {
	uint32_t i = 0, j = 0;
  unsigned char char_3[3];
  unsigned char char_4[4];
	uint32_t enc_size, count = 0;
	char *ret = NULL, *out = NULL;

	if(!setme_len || !input) {
		return NULL;
	}

	enc_size = ((len + 13) / 3) * 4;
	ret = am_malloc(enc_size);
	out = ret;

	dbg_printf(P_DBG, "Calculated encoded size: %d", enc_size);
  while (len--) {
    char_3[i++] = *(input++);
    if (i == 3) {
      char_4[0] = (char_3[0] & 0xfc) >> 2;
      char_4[1] = ((char_3[0] & 0x03) << 4) + ((char_3[1] & 0xf0) >> 4);
      char_4[2] = ((char_3[1] & 0x0f) << 2) + ((char_3[2] & 0xc0) >> 6);
      char_4[3] = char_3[2] & 0x3f;

      for(i = 0; i < 4; i++) {
				*out = alphabet[char_4[i]];
				out++;
				count++;
			}
      i = 0;
    }
  }

  if (i != 0)  {
    for(j = i; j < 3; j++) {
      char_3[j] = '\0';
		}

    char_4[0] = (char_3[0] & 0xfc) >> 2;
    char_4[1] = ((char_3[0] & 0x03) << 4) + ((char_3[1] & 0xf0) >> 4);
    char_4[2] = ((char_3[1] & 0x0f) << 2) + ((char_3[2] & 0xc0) >> 6);
    char_4[3] = char_3[2] & 0x3f;

    for (j = 0; j < i + 1; j++) {
      *out = alphabet[char_4[j]];
			out++;
			count++;
		}
    while(i++ < 3) {
      *out = '=';
			out++;
			count++;
		}
  }
	*out = '\0';
	dbg_printf(P_DBG, "Actual encoded size: %d", count);
	*setme_len = count;

  return ret;
}

static int is_base64(unsigned char c) {
  return (isalnum(c) || (c == '+') || (c == '/'));
}

static char find(char c) {
	int i;
	for(i = 0; i < 64; i++) {
		if(alphabet[i] == c) {
			return i;
		}
	}
	return -1;
}

/** \brief Decode Base64-encoded data.
 *
 * \param[in] encoded_string encoded input data
 * \param[in] in_len size of input data
 * \param[out] setme_len size of the decoded data
 * \return pointer to decoded data
 */
char *base64_decode(const char *encoded_string, uint32_t in_len, uint32_t * setme_len) {
  uint32_t i = 0;
  uint32_t j = 0;
  uint32_t in_ = 0;
  uint32_t count = 0;
  unsigned char char_array_4[4], char_array_3[3];
  char *ret = NULL, *out = NULL;

	if(!setme_len || !encoded_string) {
		return NULL;
	}

  ret = am_malloc(3 * (in_len / 4) + 2);
	out = ret;

  while (in_len-- && ( encoded_string[in_] != '=') && is_base64(encoded_string[in_])) {
    char_array_4[i++] = encoded_string[in_]; in_++;
    if (i ==4) {
      for (i = 0; i < 4; i++) {
        char_array_4[i] = find(char_array_4[i]);
			}

      char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
      char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
      char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

      for (i = 0; (i < 3); i++) {
        *out = char_array_3[i];
				out++;
				count++;
			}
      i = 0;
    }
  }

  if (i) {
    for (j = i; j <4; j++)
      char_array_4[j] = 0;

    for (j = 0; j <4; j++)
      char_array_4[j] = find(char_array_4[j]);

    char_array_3[0] = (char_array_4[0] << 2) + ((char_array_4[1] & 0x30) >> 4);
    char_array_3[1] = ((char_array_4[1] & 0xf) << 4) + ((char_array_4[2] & 0x3c) >> 2);
    char_array_3[2] = ((char_array_4[2] & 0x3) << 6) + char_array_4[3];

    for (j = 0; (j < i - 1); j++) {
			*out = char_array_3[j];
			out++;
			count++;
		}
  }
	*out = '\0';

	*setme_len = count;

  return ret;
}
