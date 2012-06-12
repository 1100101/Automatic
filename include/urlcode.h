#ifndef URLCODE_H__
#define URLCODE_H__


char from_hex(char ch);
char to_hex(char code);
char *url_encode(const char *str);
char *url_encode_whitespace(const char *str);
char *url_decode(const char *str);


#endif /* URLCODE_H__ */
