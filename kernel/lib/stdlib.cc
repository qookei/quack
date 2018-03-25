#include "stdlib.h"

int atoi(const char *str) {
	int res = 0;
	int sign = 1;
	
	while (isspace(*str)) str++;
	
	if (*str == '-') {
		sign = -1;
		str ++;
	}
	
	if (*str == '+') {
		str ++;
	}
	
	for (; *str; str++) {
		if (isdigit(*str))
			res = res * 10 + (*str - '0');
		else
			return sign * res;
	}
	
	return sign * res;
	
}

long atol(const char *str) {
	long res = 0;
	int sign = 1;
	
	while (isspace(*str)) str++;
	
	if (*str == '-') {
		sign = -1;
		str ++;
	}
	
	if (*str == '+') {
		str ++;
	}
	
	for (; *str; str++) {
		if (isdigit(*str))
			res = res * 10 + (*str - '0');
		else
			return sign * res;
	}
	
	return sign * res;
	
}

long long atoll(const char *str) {
	long long res = 0;
	int sign = 1;
	
	while (isspace(*str)) str++;
	
	if (*str == '-') {
		sign = -1;
		str ++;
	}
	
	if (*str == '+') {
		str ++;
	}
	
	for (; *str; str++) {
		if (isdigit(*str))
			res = res * 10 + (*str - '0');
		else
			return sign * res;
	}
	
	return sign * res;
}

void reverse(char* s) {
	int i, j;
	char c;

	for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
		c = s[i];
		s[i] = s[j];
		s[j] = c;
	}
}

char* itoa(uint32_t n, char* s, int base) {
	int i;
	
	i = 0;
	do {       /* generate digits in reverse order */
		s[i++] = "0123456789ABCDEF"[n % base];   /* get next digit */
	} while ((n /= base) > 0);     /* delete it */
	s[i] = '\0';
	reverse(s);
	return s;
}