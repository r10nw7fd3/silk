#ifndef _VECTOR_H_
#define _VECTOR_H_

#include <stdlib.h>
#include <assert.h>

#define VECTOR_DEFINE(type) \
	typedef struct Vector_##type Vector_##type; \
	struct Vector_##type { \
		type* data; \
		size_t capacity; \
		size_t size; \
		int(*append)(Vector_##type*, type); \
	}; \
\
	static inline int vector_##type##_append(Vector_##type* vec, type val); \
\
	static inline int vector_##type##_init(Vector_##type* vec, size_t capacity) { \
		vec->capacity = capacity; \
		vec->size = 0; \
		vec->append = vector_##type##_append; \
		vec->data = malloc(sizeof(type) * capacity); \
		return vec->data == NULL; \
	} \
	static inline void vector_##type##_ainit(Vector_##type* vec, size_t capacity) { \
		assert(!vector_##type##_init(vec, capacity)); \
	} \
\
	static inline int vector_##type##_append(Vector_##type* vec, type val) { \
		if(vec->size >= vec->capacity) { \
			type* reassign = realloc(vec->data, sizeof(type) * (vec->capacity + 20)); \
			if(!reassign) \
				return 1; \
			vec->data = reassign; \
			vec->capacity += 20; \
		} \
		vec->data[vec->size++] = val; \
		return 0; \
	}

#define vector_append(vec, val) (vec)->append(vec, val)
#define vector_aappend(vec, val) assert(!(vec)->append(vec, val))

#define vector_deinit(vec) ( \
        free((vec)->data), \
        (vec)->capacity = 0, \
        (vec)->size = 0 \
        )

#endif
