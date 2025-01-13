#include "vector.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// Create a new vector
Vector *vector_create(size_t element_size)
{
    Vector *vec = (Vector *)malloc(sizeof(Vector));
    if (!vec)
    {
        perror("Failed to allocate vector");
        exit(EXIT_FAILURE);
    }

    vec->data = NULL;
    vec->element_size = element_size;
    vec->size = 0;
    vec->capacity = 0;

    return vec;
}

// Free the vector
void vector_free(Vector *vec)
{
    if (vec)
    {
        free(vec->data);
        free(vec);
    }
}

// Ensure enough capacity, resizing if necessary
static void vector_ensure_capacity(Vector *vec, size_t min_capacity)
{
    if (vec->capacity < min_capacity)
    {
        size_t new_capacity = (vec->capacity == 0) ? 1 : vec->capacity * 2;
        if (new_capacity < min_capacity)
        {
            new_capacity = min_capacity;
        }

        void *new_data = realloc(vec->data, new_capacity * vec->element_size);
        if (!new_data)
        {
            perror("Failed to reallocate vector data");
            exit(EXIT_FAILURE);
        }

        vec->data = new_data;
        vec->capacity = new_capacity;
    }
}

// Add an element to the end of the vector
void vector_push_back(Vector *vec, const void *element)
{
    vector_ensure_capacity(vec, vec->size + 1);
    memcpy((char *)vec->data + vec->size * vec->element_size, element, vec->element_size);
    vec->size++;
}

// Remove the last element from the vector
void vector_pop_back(Vector *vec)
{
    if (vec->size > 0)
    {
        vec->size--;
    }
}

// Get a pointer to an element at a specific index
void *vector_get(const Vector *vec, size_t index)
{
    if (index >= vec->size)
    {
        fprintf(stderr, "Index out of bounds: %zu\n", index);
        exit(EXIT_FAILURE);
    }

    return (char *)vec->data + index * vec->element_size;
}

// Resize the vector to a new size
void vector_resize(Vector *vec, size_t new_size)
{
    if (new_size > vec->capacity)
    {
        vector_ensure_capacity(vec, new_size);
    }
    if (new_size > vec->size)
    {
        memset((char *)vec->data + vec->size * vec->element_size, 0,
               (new_size - vec->size) * vec->element_size);
    }
    vec->size = new_size;
}

// Get the current size of the vector
size_t vector_size(const Vector *vec)
{
    return vec->size;
}

// Get the current capacity of the vector
size_t vector_capacity(const Vector *vec)
{
    return vec->capacity;
}

void buf_append(Vector *buf, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; i++)
    {
        vector_push_back(buf, &data[i]);
    }
}

void buf_consume(Vector *buf, size_t n)
{
    if (n >= vector_size(buf))
    {
        // If consuming more or equal to the size, clear the buffer
        vector_resize(buf, 0);
    }
    else
    {
        // Shift elements to the front
        memmove(buf->data, (uint8_t *)buf->data + n, (vector_size(buf) - n) * buf->element_size);
        // Adjust the size of the vector
        vector_resize(buf, vector_size(buf) - n);
    }
}