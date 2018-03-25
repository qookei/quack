#include "string.h"

size_t strlen(const char *s) {
	size_t len = 0;
	while(s[len]) {
		len++;
	}
	return len;
}

int strcmp(const char *str1, const char *str2) {
	size_t i = 0;
	while (str1[i] && str1[i] == str2[i]) i++;

	return str1[i] - str2[i];
}

int strncmp(const char *str1, const char *str2, size_t len) {
	size_t i = 0;
	while (i < len - 1 && str1[i] == str2[i]) i++;
	return str1[i] - str2[i];
}

char *strcpy(char *dest, const char *src) {
	size_t i;
	for (i=0; src[i]; i++) {
		dest[i] = src[i];
	}
	dest[i] = '\0';
	
	return dest;
}

char *strncpy(char *dest, const char *src, size_t len) {
	size_t i;
	
	for (i = 0; src[i] && (i < len); i++)
		dest[i] = src[i];
		
	if (i < len)
		for(; i < len; i++)
			dest[i] = '\0';
	
	return dest;
}

char *strcat(char *dest, const char *src) {
	char *ret = dest;
	
	while(*dest++);
	
	strcpy(--dest, src);
	
	return ret;
}

char *strncat(char *dest, const char *src, size_t len) {
	char *ret = dest;
	
	while(*dest++);
	
	strncpy(--dest, src, len);
	
	dest[len] = '\0';
	
	return ret;
}

char *strchr(char *str, int ch) {
	for (; *str; str++)
		if(*str==(char)ch)
			return str;
			
	return (char*)0;
}

char *strrchr(char *str, int ch) {
	char *sp = (char*)0;
	
	for (; *str; str++)
		if(*str==(char)ch)
			sp = str;
			
	return sp;
}

char *strstr(char *str1, const char *str2) {
	for (size_t i = 0; *str1; str1++, i = 0) {
		if (*str1 == *str2) {
			for (i = 1; str1[i] == str2[i];) {
				i++;
				if (!str2[i]) return str1;
				if (!str1[i]) return (char*)0;
			}
		}
	}
	
	return NULL;
}

char *strpbrk(char *str, char *targets) {
	for (; *str; str++) {
		for (char *sp = targets; *sp; sp++) {
			if (*str == *sp) return sp;
		}
	}
	return (char*)0;
}

void *memset(void *arr, int val, size_t len) {
	for (size_t i = 0; i < len; i++)
		((unsigned char*)arr)[i] = (unsigned char)val;
		
	return arr;
}

int memcmp(const void *m1, const void *m2, size_t len) {
	size_t i;
	for (i = 0; i< len; i++) {
		if (((unsigned char*)m1)[i] > ((unsigned char*)m2)[i])
			return 1;
			
		if (((unsigned char*)m1)[i] < ((unsigned char*)m2)[i])
			return -1;
	}
	return 0;
}

void *memcpy(void *dest, const void *src, size_t len) {
	for (size_t i = 0; i < len; i++) 
		((unsigned char*)dest)[i] = ((unsigned char*)src)[i];
		
	return dest;
}

void *memmove(void *dest, const void *src, size_t len) {
	unsigned char cpy[len];
	memcpy(cpy, src, len);
	return memcpy(dest, cpy, len);
}

void *memchr(void *haystack, int needle, size_t size) {
	char *h = (char *)haystack;
	for (; size--; h++) {
		if (*h == (char)needle)
			return (void*)h;		
	}
	
	return (void*)0;
}