#include <stdlib.h>
#include <assert.h>

#define VECTOR_DEFINE(type) \
        typedef struct { \
                type _typename; \
                type* _reassign; \
                type* data; \
                size_t capacity; \
                size_t size; \
        } Vector_##type

#define vector_init(vec, cap) ( \
        (vec).data = malloc(sizeof((vec)._typename) * cap), \
        (vec).data \
                ? (vec).capacity = cap, (vec).size = 0, 0 \
                : 1 \
        )
#define vector_ainit(vec, cap) assert(!vector_init(vec, cap))

#define vector_deinit(vec) ( \
        free((vec).data), \
        (vec).capacity = 0, \
        (vec).size = 0 \
        )

#define vector_append(vec, val) ( \
        (vec).size + 1 > (vec).capacity \
                ? (vec)._reassign = realloc((vec).data, \
                        ((vec).capacity + 20) * sizeof((vec)._typename)), \
                        (vec)._reassign \
                                ? (vec).data = (vec)._reassign, \
                                        (vec).capacity += 20, \
                                        (vec).data[(vec).size++] = (val), \
                                        0 \
                                : 1 \
                : (((vec).data[(vec).size++] = (val)), 0) \
        )
#define vector_aappend(vec, val) assert(!vector_append(vec, val))
