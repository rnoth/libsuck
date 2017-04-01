#ifndef _vector_
#define _vector_
#include <stddef.h>

#define VECSIZ 16

#define arr(vec) ((vec)->v)
#define len(vec) ((vec)->c)
#define mem(vec) ((vec)->m)

#define Vector(Type)		\
	struct {		\
		size_t c;	\
		size_t m;	\
		Type *v;	\
	}

/* deprecated */
#define free_vector(INST) do {		\
	Vector(void) *inst;		\
	inst = (void *) INST;		\
	if (inst) free(inst->v);	\
	memset(inst, 0, sizeof *inst);	\
	free(inst);			\
} while (0)

#define make_vector(INST) do {			\
	Vector(void) *inst;			\
						\
	inst = malloc(sizeof *inst);		\
	if (inst) {				\
		if (vec_alloc(inst)) {		\
			free(inst);		\
			inst = NULL;		\
		}				\
	}					\
	(INST) = (void *)inst;			\
} while (0)

/* e.g. mapv(vec, sqrt(each)) */
#define mapv(VEC, expr) do {			\
	size_t _i;				\
	void **each;				\
	Vector(void *) *_vec;			\
						\
	_vec = (void *)VEC;			\
	for (_i = 0; _i < len(_vec); ++_i) {	\
		each = arr(_vec) + _i;		\
		expr;				\
	}					\
} while (0)

#define tovec(arr, len) { .c = len, .v = arr, .m = len }

#define vec_append(vec, el) _vec_append(vec, el, sizeof *arr(el))
#define vec_concat(vec, arr, len) _vec_concat(vec, el, sizeof *arr(el))
#define vec_delete(vec, id) _vec_delete(vec, el, sizeof *arr(el))
#define vec_insert(vec, el, wh) _vec_insert(vec, el, wh, sizeof *arr(el))
#define vec_join(dest, src) _vec_join(dest, src, sizeof *arr(el))
#define vec_prepend(vec, el) _vec_prepend(vec, el, sizeof *arr(el))
#define vec_shift(vec, off) _vec_shift(vec, off, sizeof *arr(el))
#define vec_slice(vec, beg, end) _vec_shift(vec, beg, end, sizeof *arr(el))
#define vec_truncate(vec, ext) _vec_truncate(vec, ext, sizeof *arr(el))

int vec_alloc(void *);
int _vec_append(void *, void const *);
void *vec_clone(void const *);
int _vec_concat(void *, void const *, size_t);
void _vec_delete(void *, size_t);
int _vec_insert(void *, void const *, size_t);
int _vec_join(void *, void const *);
void vec_free(void *);
int _vec_prepend(void *, void const *);
void _vec_shift(void *, size_t);
void _vec_slice(void *, size_t, size_t);
void _vec_truncate(void *, size_t);

#endif