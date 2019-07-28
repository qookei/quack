#define ELF_IDENT_CLASS 4
#define ELF_IDENT_DATA 5

#define ELF_MAGIC "\177ELF" // TODO

#define ELF_CLASS_32 1
#define ELF_CLASS_64 2

#define ELF_DATA_LSB 1
#define ELF_DATA_MSB 2

#define ELF_TYPE_EXEC 2

#define ELF_VER_CURRENT 1

#define ELF_PHDR_TYPE_NULL 0
#define ELF_PHDR_TYPE_LOAD 1
#define ELF_PHDR_TYPE_NOTE 4
#define ELF_PHDR_TYPE_PHDR 6

#define ELF_PHDR_FLAG_X (1 << 0)
#define ELF_PHDR_FLAG_W (1 << 1)
#define ELF_PHDR_FLAG_R (1 << 2)
