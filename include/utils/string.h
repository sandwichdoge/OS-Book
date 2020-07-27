#ifdef __cplusplus
extern "C" {
#endif

#ifndef INCLUDE_STRING_H
#define INCLUDE_STRING_H
#include "stddef.h"
#include "stdint.h"
int _strncmp(char *s1, char *s2, unsigned int len);

void _memset_s(char *__restrict dst, char c, unsigned int len);
void _memset_u(unsigned char *__restrict dst, char c, unsigned int len);

void _memcpy_s(char *__restrict dst, char *__restrict src, unsigned int len);
void _memcpy_u(unsigned char *__restrict dst, char *__restrict src, unsigned int len);

int _strcpy_s(char *__restrict dst, char *__restrict src);
int _strcpy_u(unsigned char *__restrict dst, char *__restrict src);

int _strncpy_s(char *__restrict dst, char *__restrict src, unsigned int maxlen);
int _strncpy_u(unsigned char *__restrict dst, unsigned char *__restrict src, unsigned int maxlen);

unsigned int _strlen_u(unsigned char *__restrict s);
unsigned int _strlen_s(char *__restrict s);

int _int_to_str_s(char *out, size_t bufsize, int n);
int _int_to_str_u(char *out, size_t bufsize, unsigned int n);

char *_int_to_str_static(unsigned int num, int base);

#define _strlen(src) _Generic((src), char * : _strlen_s, default : _strlen_s, unsigned char * : _strlen_u)(src)

#define _strcpy(dst, src) _Generic((src), char * : _strcpy_s, default : _strcpy_s, unsigned char * : _strcpy_u)(dst, src)

#define _strncpy(dst, src, len) _Generic((src), char * : _strncpy_s, default : _strncpy_s, unsigned char * : _strncpy_u)(dst, src, len)

#define _memcpy(dst, src, len) _Generic((src), char * : _memcpy_s, default : _memcpy_s, unsigned char * : _memcpy_u)(dst, src, len)

#define _memset(dst, c, len) _Generic((dst), char * : _memset_s, default : _memset_s, unsigned char * : _memset_u)(dst, c, len)

#define _int_to_str(out, bufsize, n) _Generic((n), int : _int_to_str_s, default : _int_to_str_s, unsigned int : _int_to_str_u)(out, bufsize, n)
#endif

#ifdef __cplusplus
}
#endif
