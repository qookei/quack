#include <stddef.h>
#include <stdint.h>
#include <kmesg.h>

enum type_kinds { TK_INTEGER = 0x0000, TK_FLOAT = 0x0001, TK_UNKNOWN = 0xffff };

struct type_descriptor {
	uint16_t type_kind;
	uint16_t type_info;
	char type_name[];
};

struct source_location {
	char *filename;
	uint32_t line;
	uint32_t column;
};

struct overflow_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct shift_out_of_bounds_data {
	struct source_location loc;
	struct type_descriptor *lhs_type;
	struct type_descriptor *rhs_type;
};

struct out_of_bounds_data {
	struct source_location loc;
	struct type_descriptor *array_type;
	struct type_descriptor *index_type;
};

struct non_null_return_data {
	struct source_location attr_loc;
};

struct type_mismatch_data_v1 {
	struct source_location loc;
	struct type_descriptor *type;
	unsigned char log_alignment;
	unsigned char type_check_kind;
};

struct type_mismatch_data {
	struct source_location loc;
	struct type_descriptor *type;
	unsigned long alignment;
	unsigned char type_check_kind;
};

struct vla_bound_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct invalid_value_data {
	struct source_location loc;
	struct type_descriptor *type;
};

struct unreachable_data {
	struct source_location loc;
};

struct nonnull_arg_data {
	struct source_location loc;
};

static void log_location(struct source_location loc) {
	kmesg("ubsan", "ubsan failure at %s:%d", loc.filename, loc.line);
}

void __ubsan_handle_add_overflow(struct overflow_data *data, uintptr_t lhs,
				uintptr_t rhs) {
	kmesg("ubsan", "add of %ld and %ld will overflow", lhs, rhs);
	log_location(data->loc);
}

void __ubsan_handle_sub_overflow(struct overflow_data *data, uintptr_t lhs,
				uintptr_t rhs) {
	kmesg("ubsan", "subtract of %ld and %ld will overflow", lhs, rhs);
	log_location(data->loc);
}

void __ubsan_handle_pointer_overflow(struct overflow_data *data, uintptr_t lhs,
				uintptr_t rhs) {
	kmesg("ubsan", "pointer %lx and %lx will overflow", lhs, rhs);
	log_location(data->loc);

}

void __ubsan_handle_mul_overflow(struct overflow_data *data, uintptr_t lhs,
				uintptr_t rhs) {
	kmesg("ubsan", "multiply of %ld and %ld will overflow", lhs, rhs);
	log_location(data->loc);
}

void __ubsan_handle_divrem_overflow(struct overflow_data *data,
	uintptr_t lhs, uintptr_t rhs) {
	kmesg("ubsan", "division of %ld and %ld will overflow", lhs, rhs);
	log_location(data->loc);
}

void __ubsan_handle_negate_overflow(struct overflow_data *data,
	uintptr_t old) {
	kmesg("ubsan", "negation of %ld will overflow", old);
	log_location(data->loc);
}

void __ubsan_handle_shift_out_of_bounds(
	struct shift_out_of_bounds_data *data, uintptr_t lhs, uintptr_t rhs) {
	kmesg("ubsan", "shift of %ld by %ld will go out of bounds", lhs, rhs);
	log_location(data->loc);
}

void __ubsan_handle_out_of_bounds(struct out_of_bounds_data *data,
				uintptr_t index) {
	kmesg("ubsan", "out of bounds access at index %ld", index);
	log_location(data->loc);

}

void __ubsan_handle_nonnull_return(struct non_null_return_data *data,
				struct source_location *loc) {
	kmesg("ubsan", "null return at %s:%d", data->attr_loc.filename, data->attr_loc.line);
	log_location(*loc);
}

void __ubsan_handle_type_mismatch_v1(struct type_mismatch_data_v1 *data,
	uintptr_t ptr) {
	if(!ptr) {
		kmesg("ubsan", "null pointer access");
	} else if (ptr & ((1 << data->log_alignment) - 1)) {
		kmesg("ubsan", "misaligned access (ptr %016lx, alignment %ld)", ptr, (1 << data->log_alignment));
	} else {
		kmesg("ubsan", "too large access (ptr %016lx)", ptr);
	}
	log_location(data->loc);
}

void __ubsan_handle_vla_bound_not_positive(struct vla_bound_data *data,
	uintptr_t bound) {
	kmesg("ubsan", "negative vla bound %ld", bound);
	log_location(data->loc);
}

void __ubsan_handle_load_invalid_value(struct invalid_value_data *data,
	uintptr_t val) {
	kmesg("ubsan", "invalid value %lx", val);
	log_location(data->loc);
}

void __ubsan_handle_builtin_unreachable(struct unreachable_data *data) {
	kmesg("ubsan", "reached __builtin_unreachabe");
	log_location(data->loc);
}

void __ubsan_handle_nonnull_arg(struct nonnull_arg_data *data) {
	kmesg("ubsan", "null argument");
	log_location(data->loc);
}

void __ubsan_handle_type_mismatch(struct type_mismatch_data *data, uintptr_t ptr) {
	if(!ptr) {
		kmesg("ubsan", "null pointer access");
	} else if (ptr & (data->alignment - 1)) {
		kmesg("ubsan", "misaligned access (ptr %016lx, alignment %ld)", ptr, data->alignment);
	} else {
		kmesg("ubsan", "too large access (ptr %016lx)", ptr);
	}
	log_location(data->loc);
}
