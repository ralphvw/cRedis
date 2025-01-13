#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h> // For size_t

typedef struct
{
    void *data;          // Pointer to the underlying data
    size_t element_size; // Size of each element
    size_t size;         // Current number of elements
    size_t capacity;     // Current allocated capacity
} Vector;

// Function declarations
Vector *vector_create(size_t element_size);
void vector_free(Vector *vec);
void vector_push_back(Vector *vec, const void *element);
void vector_pop_back(Vector *vec);
void *vector_get(const Vector *vec, size_t index);
void vector_resize(Vector *vec, size_t new_size);
size_t vector_size(const Vector *vec);
size_t vector_capacity(const Vector *vec);
void buf_consume(Vector *buf, size_t n);
void buf_append(Vector *buf, const uint8_t *data, size_t len);

#endif // VECTOR_H