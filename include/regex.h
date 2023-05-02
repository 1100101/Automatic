#include <stdint.h>

uint8_t isRegExMatch(const char* pattern, const char* str);
char* getRegExMatch(const char* pattern, const char* str, uint8_t which_result);
char * performRegexReplace(const char* str, const char* pattern, const char* replace);

