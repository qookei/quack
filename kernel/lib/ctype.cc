#include "ctype.h"

int isalnum(int ch) {
	return isalpha(ch) || isdigit(ch);
}

int isalpha(int ch) {
	return isupper(ch) || islower(ch);
}

int isblank(int ch) {
	return ch == 0x09 || ch == 0x20;
}

int iscntrl(int ch) {
	return (ch >= 0x00 && ch <= 0x1F) || ch == 0x7F;
}

int isdigit(int ch) {
	return ch >= 0x30 && ch <= 0x39;
}

int isgraph(int ch) {
	return ch >= 0x21 && ch <= 0x7A;
}

int islower(int ch) {
	return ch >= 0x61 && ch <= 0x7A;
}

int isprint(int ch) {
	return ch == 0x20 || isgraph(ch);
}

int ispunct(int ch) {
	return (ch >= 0x21 && ch <= 0x2F) ||
			(ch >= 0x3A && ch <= 0x40) ||
			(ch >= 0x5B && ch <= 0x60) ||
			(ch >= 0x7B && ch <= 0x7E);
}

int isspace(int ch) {
	return ch == 0x20 || (ch >= 0x09 && ch <= 0x0D);
}

int isupper(int ch) {
	return ch >= 0x41 && ch <= 0x5A;
}

int isxdigit(int ch) {
	return (ch >= 0x41 && ch <= 0x46) || (ch >= 0x61 && ch <= 0x66) || isdigit(ch);
}

int tolower(int ch) {
	if (isupper(ch))
		return ch ^ 0x20;
	return ch;
}

int toupper(int ch) {
	if (islower(ch))
		return ch ^ 0x20;
	return ch;
}