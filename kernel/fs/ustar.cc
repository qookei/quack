#include "ustar.h"

extern int printf(const char *, ...);

size_t oct_to_dec(char *string) {
	size_t integer = 0;
	size_t multiplier = 1;
	size_t i = strlen(string) - 1;

	while(i >= 0 && string[i] >= '0' && string[i] <= '7') {
		integer += (string[i] - 48) * multiplier;
		multiplier *= 8;
		i--;
	}

	return integer;
}

uint64_t ustar_get_file(mountpoint_t *mountpoint, const char *path, ustar_entry_t *destination) {

	uint64_t block = 0;
	size_t file_size;

	struct stat s;
	int st = stat(mountpoint->dev, &s);

	if (st < 0) {
		printf("ustar: stat failed!\n");
		return 1;
	}

	char *buffer = (char *)kmalloc(s.st_size);

	int handle = open(mountpoint->dev, O_RDONLY);

	if (handle < 0) {
		printf("ustar: open failed!\n");
		return 1;
	}

	int r = read(handle, buffer, s.st_size);
	
	if (!r) {
		printf("ustar: read failed!\n");
		return 1;
	}

	if (r != s.st_size) {
		printf("ustar: read %i bytes, while file size is %i bytes\n", r, s.st_size);
		return 1;
	}

	close(handle);

	ustar_entry_t *entry = (ustar_entry_t *)buffer;

	while(1) {

		if ((uint32_t)entry > (uint32_t)buffer + s.st_size) {
			break;
		}

		if (memcmp(entry->signature, "ustar", 5) != 0) {
			break;
		}

		if(strlen(entry->name) == strlen(path) && !memcmp(entry->name, path, strlen(path))) {
			memcpy(destination, entry, sizeof(ustar_entry_t));
			kfree(buffer);
			// printf("found %s\n", entry->name);
			return block * USTAR_BLOCK_SIZE;
		}

		file_size = oct_to_dec(entry->size);
		block += (file_size + USTAR_BLOCK_SIZE - 1) / USTAR_BLOCK_SIZE;
		block++;
		entry = (ustar_entry_t*)(buffer + (block * USTAR_BLOCK_SIZE));
		
	}

	kfree(buffer);

	return 1;
}



int ustar_read(mountpoint_t *mountpoint, const char *path, char *buffer, size_t count) {

	ustar_entry_t entry;
	uint64_t offset = ustar_get_file(mountpoint, path, &entry);
	if(offset == 1)
		return ENOENT;

	struct stat s;
	stat(mountpoint->dev, &s);
	char *_buffer = (char *)kmalloc(s.st_size);
	
	int handle = open(mountpoint->dev, O_RDONLY);
	int c = read(handle, _buffer, s.st_size);
	close(handle);

	char *data = _buffer + offset + 512;

	size_t size = oct_to_dec(entry.size);

	if (size < count) {
		memcpy(buffer, data, size);
		kfree(buffer);
		return size;
	} else {
		memcpy(buffer, data, count);
		kfree(buffer);
		return count;
	}

	return EINVAL;
}



int ustar_write(mountpoint_t *, const char *, char *, size_t) {
	return EINVAL;
}

int ustar_stat(mountpoint_t *mnt, const char *path, struct stat *s) {
	
	ustar_entry_t entry;
	uint64_t offset = ustar_get_file(mnt, path, &entry);
	if (offset == 1)
		return ENOENT;

	s->st_dev = 0;
	
	s->st_ino = offset / USTAR_BLOCK_SIZE;
	s->st_nlink = 0;
	s->st_uid = oct_to_dec(entry.uid);
	s->st_gid = oct_to_dec(entry.gid);
	s->st_size = oct_to_dec(entry.size);
	s->st_mtime = oct_to_dec(entry.mtime);
	s->st_ctime = oct_to_dec(entry.mtime);
	s->st_atime = 0;
	s->st_blksize = USTAR_BLOCK_SIZE;
	s->st_blocks = (s->st_size + USTAR_BLOCK_SIZE - 1) / USTAR_BLOCK_SIZE;

	s->st_mode = 0;

	switch (entry.type) {
	case USTAR_REG:
	case 0:
		s->st_mode |= S_IFREG;
		break;
	case USTAR_HARD_LINK:
	case USTAR_SYMLINK:
		s->st_mode |= S_IFLNK;
		break;
	case USTAR_CHR:
		s->st_mode |= S_IFCHR;
		break;
	case USTAR_BLK:
		s->st_mode |= S_IFBLK;
		break;
	case USTAR_DIR:
		s->st_mode |= S_IFDIR;
		break;
	case USTAR_FIFO:
		s->st_mode |= S_IFIFO;
		break;
	default:
		printf("ustar: %s: unknown file type %xb, ignoring...\n", path, entry.type);
		break;
	}

	size_t permissions = oct_to_dec(entry.mode);
	if (permissions & USTAR_READ_USER)
		s->st_mode |= S_IRUSR;

	if (permissions & USTAR_WRITE_USER)
		s->st_mode |= S_IWUSR;

	if (permissions & USTAR_EXECUTE_USER)
		s->st_mode |= S_IXUSR;

	if (permissions & USTAR_READ_GROUP)
		s->st_mode |= S_IRGRP;

	if (permissions & USTAR_WRITE_GROUP)
		s->st_mode |= S_IWGRP;

	if (permissions & USTAR_EXECUTE_GROUP)
		s->st_mode |= S_IXGRP;

	if (permissions & USTAR_READ_OTHER)
		s->st_mode |= S_IROTH;

	if (permissions & USTAR_WRITE_OTHER)
		s->st_mode |= S_IWOTH;

	if (permissions & USTAR_EXECUTE_OTHER)
		s->st_mode |= S_IXOTH;

	return 0;
}