#include "uuid.h"

// this randomness is poor, but should be good enough just to generate
// random identification uuids
static uint64_t uuid_internal_next = 1;
static uint32_t uuid_internal_rand() {
    uuid_internal_next = uuid_internal_next * 1103515245 + 12345;
    return (uint32_t)(uuid_internal_next / 65536) % 32768;
}

static char *uuid_internal_itoa(uint8_t byte, char *dst) {
	char *ptr = dst + 1;
	*ptr-- = "0123456789ABCDEF"[byte % 16];
	byte /= 16;
	*ptr = "0123456789ABCDEF"[byte % 16];
	return dst + 2;
}

void uuid_generate(uint64_t seed, uint8_t *dest) {
	uuid_internal_next = seed;
	
	for (size_t i = 0; i < 16; i++) {
		dest[i] = uuid_internal_rand();
	}

	dest[8] &= 0xBF;
	dest[8] |= 0x80;

	dest[6] &= 0x4F;
	dest[6] |= 0x40;
}

// uuids are big endian and have the following format
// aabbccdd-eeff-gghh-iijj-kkllmmnnoopp
void uuid_to_string(char *dest, uint8_t *uuid) {
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	*dest++ = '-';
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	*dest++ = '-';
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	*dest++ = '-';
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	*dest++ = '-';
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
	dest = uuid_internal_itoa(*uuid++, dest);
}
