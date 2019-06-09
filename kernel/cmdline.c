#include "cmdline.h"
#include <mm/heap.h>
#include <string.h>
#include <kmesg.h>

typedef struct {
	char *key;
	char **values;
	size_t value_count;
} cmd_arg_t;

static size_t count_chars(char *str, char chr) {
	size_t count = 0;
	
	while (*str) {
		if (*str == chr)
			count++;
		str++;
	}

	return count;
}

static cmd_arg_t *arg_list = NULL;
static size_t arg_count = 0;

static void parse_arg(char *str) {
	cmd_arg_t arg;

	char *name = str;
	char *params = strchr(str, '=');

	arg.value_count = 0;
	arg.values = NULL;

	if (params) {
		*params = '\0';
		params++;

		size_t param_count = count_chars(params, ',') + 1;

		if (*params != '\0') {
			arg.value_count = param_count;
			arg.values = kcalloc(param_count, sizeof (char *));
			size_t idx = 0;

			char *param = params;
			while (param_count--) {
				char *comma = strchr(param, ',');
				if (!comma)
					comma = param + strlen(param);

				size_t len = comma - param;
				arg.values[idx] = kcalloc(len + 1, 1);
				memcpy(arg.values[idx], param, len);

				param = strchr(param, ',') + 1;
				idx++;
			}
		}
	}

	arg.key = kcalloc(strlen(name) + 1, 1);	
	strcpy(arg.key, name);

	memcpy(&arg_list[arg_count], &arg, sizeof(arg));
	arg_count++;
}

void cmdline_init(char *cmdline) {
	char *c = cmdline;
	
	size_t cmd_arg_count = count_chars(cmdline, ' ') + 1;
	arg_list = kcalloc(cmd_arg_count, sizeof(cmd_arg_t));

	while (cmd_arg_count--) {
		if (!(*c))
			break;
		
		size_t len = 0;
		char *sep = strchr(c, ' ');
		if (!sep)
			len = strlen(c);
		else
			len = sep - c;

		char *buf = kcalloc(len + 1, 1);
		memcpy(buf, c, len);
		parse_arg(buf);
		kfree(buf);

		c = strchr(c, ' ') + 1;
	}

	for (size_t i = 0; i < arg_count; i++) {
		cmd_arg_t a = arg_list[i];
		if (a.value_count) {
			kmesg("cmdline", "flag '%s' is set and has %u value(s) assigned:", a.key, a.value_count);
			for (size_t j = 0; j < a.value_count; j++) {
				kmesg("cmdline", "\t\t'%s'%s", a.values[j], j != a.value_count - 1 ? "," : "");
			}
		} else {
			kmesg("cmdline", "flag '%s' is set and has no values assigned", a.key);
		}
	}
}

int cmdline_is_enabled(const char *name) {
	for (size_t i = 0; i < arg_count; i++) {
		if (!strcmp(name, arg_list[i].key))
			return 1;
	}

	return 0;
}

size_t cmdline_get_value_count(const char *name) {
	for (size_t i = 0; i < arg_count; i++) {
		if (!strcmp(name, arg_list[i].key))
			return arg_list[i].value_count;
	}

	return 0;
}

char **cmdline_get_values(const char *name) {
	for (size_t i = 0; i < arg_count; i++) {
		if (!strcmp(name, arg_list[i].key))
			return arg_list[i].values;
	}

	return NULL;
}

int cmdline_has_value(const char *name, const char *value) {
	for (size_t i = 0; i < arg_count; i++) {
		if (!strcmp(name, arg_list[i].key)) {
			for (size_t j = 0; j < arg_list[i].value_count; j++) {
				if (!strcmp(value, arg_list[i].values[j]))
					return 1;
			}
		}
	}

	return 0;
}
