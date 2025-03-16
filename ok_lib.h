/*
 ok-lib
 https://github.com/brackeen/ok-lib
 Copyright (c) 2016-2020 David Brackeen

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software and
 associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT
 NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT
 OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

#ifndef OK_LIB_H
#define OK_LIB_H

/**
 @file
 Generic vector and hash maps for C. Requires C99 or newer, works best with C11.

 By default, including "ok_lib.h" defines the ok_lib functions as `static` (that is, private to the
 .c file that included it).

 ## Options - defined before #include "ok_lib.h"

 | Option                        | Description                                                     |
 |-------------------------------|-----------------------------------------------------------------|
 | #define OK_LIB_DEFINE         | Define the functions publicly, so they are available when       |
 |                               | linking.                                                        |
 |-------------------------------|-----------------------------------------------------------------|
 | #define OK_LIB_DECLARE        | Declare function prototypes without defining the functions      |
 |                               | themselves (that is, use existing public `ok_lib` functions).   |
 |-------------------------------|-----------------------------------------------------------------|
 | #define OK_LIB_USE_STDATOMIC  | Force usage of <stdatomic.h>. If not defined, `ok_lib` checks   |
 |                               | the compiler version to determine whether to use it.            |
 |-------------------------------|-----------------------------------------------------------------|

 */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h> // free, qsort
#include <string.h> // strcmp, memset, memcpy

// @cond configuration
#if defined(OK_LIB_DEFINE) || defined(OK_LIB_DECLARE)
#  ifdef __cplusplus
#    define OK_LIB_API extern "C"
#  else
#    define OK_LIB_API
#  endif
#else
#  define OK_LIB_API static
#  define OK_LIB_DEFINE
#endif
#ifdef __cplusplus
#  define OK_MUTABLE mutable
#else
#  define OK_MUTABLE
#endif
// @endcond

// MARK: Static assertion

/**
 Static assertion, using the built-in _Static_assert if available.

 @param condition The constant expression to check. If 0, a compile-time error occurs.
 @param msg       The message to display if compilation fails. Only works on C11 or C++11.
 */
#if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#  define ok_static_assert _Static_assert
#elif defined(__cplusplus) && (__cplusplus >= 201103L)
#  define ok_static_assert static_assert
#else
#  define ok_static_assert(condition, msg) ((void) sizeof(char[(condition) ? 1 : -1]))
#endif

/**
 Check if two variables have the same type.
 */
#if defined(__GNUC__)
#  define ok_types_compatible(A, B) __builtin_types_compatible_p(__typeof(A), __typeof(B))
#else
#  define ok_types_compatible(A, B) (sizeof(A) == sizeof(B))
#endif

// MARK: Vector

/**
 Declares a generic `ok_vec` struct or typedef.
 
 For example, and array of `int`s can be declared as a typedef:

     typedef struct ok_vec_of(int) vec_int_t;

 or a struct:

     struct vec_int_s ok_vec_of(int);

 @tparam value_type The type to contain in the vector.

 @return `{ value_type *values; size_t count; size_t capacity; }`
 */
#define ok_vec_of(value_type) \
    { value_type *values; size_t count; size_t capacity; }

/**
 A macro to initialize a vector statically.

     typedef struct ok_vec_of(int) vec_int_t;
     vec_int_t int_vec = OK_VEC_INIT;

 When finished using the vector, the #ok_vec_deinit() function must be called.
 */
#define OK_VEC_INIT { NULL, 0, 0 }

/**
 Inits a vector.
 
 When finished using the vector, the #ok_vec_deinit() function must be called.

 @param vec Pointer to the vector.
 */
#define ok_vec_init(vec) \
    memset((vec), 0, sizeof(*(vec)))

/**
 Deinits the vector. The vector may be used again by calling #ok_vec_init().

 @param vec Pointer to the vector.
 */
#define ok_vec_deinit(vec) \
    free((void *)(vec)->values)

/**
 Removes all elements from the vector, setting the count to 0. The capacity of the vector is not
 changed.

 @param vec Pointer to the vector.
 */
#define ok_vec_clear(vec) \
    (vec)->count = 0

/**
 Gets the number of elements in the vector.

 @param vec Pointer to the vector.
 @return size_t The number of elements in the vector.
 */
#define ok_vec_count(vec) \
    (vec)->count

/**
 Gets a pointer to the first element in a vector.

 @param vec Pointer to the vector.
 @return A pointer to the first element. If the vector is empty, the result is undefined.
 */
#define ok_vec_begin(vec) \
    ((vec)->values)

/**
 Gets a pointer to the element following the last element in a vector.
 
 The pointer should only be used for iteration, and should not be accessed directly.

 @param vec Pointer to the vector.
 @return A pointer to the element following the last element.
 */
#define ok_vec_end(vec) \
    ((vec)->values + (vec)->count)

/**
 Gets a pointer to the last element in a vector.

 @param vec Pointer to the vector.
 @return A pointer to the last element. If the vector is empty, the result is undefined.
 */
#define ok_vec_last(vec) \
    ((vec)->values + ((vec)->count - 1))

/**
 Gets an element in a vector.

 @param vec Pointer to the vector.
 @param i The index in the vector, from 0 to `(ok_vec_count(vec) - 1)`. If the index is outside of
 the valid range, the result is undefined, and this function may crash.
 @return The element at the index.
 */
#define ok_vec_get(vec, i) \
    (*((vec)->values + (i)))

/**
 Gets a pointer to an element in a vector.

 @param vec Pointer to the vector.
 @param i The index in the vector, from 0 to `(ok_vec_count(vec) - 1)`. If the index is outside of
 the valid range, the result is undefined.
 @return A pointer to the element at the index.
 */
#define ok_vec_get_ptr(vec, i) \
    ((vec)->values + (i))

/**
 Adds a value to the end of a vector.

 @param vec Pointer to the vector.
 @param value The value to add.
 @return `true` if the value was successfully added to the vector, `false` otherwise (out of memory
 error).
 */
#define ok_vec_push(vec, value) \
    (ok_vec_ensure_capacity(vec, 1) ? ((vec)->values[((vec)->count++)] = (value), 1) : 0)

/**
 Adds space for a value at the end of a vector, returning a pointer to the newly added location.

 @param vec Pointer to the vector.
 @return The pointer to the new location, or `NULL` if failure (out of memory error).
 */
#define ok_vec_push_new(vec) \
    (ok_vec_ensure_capacity(vec, 1) ? ((vec)->values + ((vec)->count++)) : NULL)

/**
 Adds all the elements from one vector into another.

 @param vec Pointer to the vector to add elements to.
 @param vec2 Pointer to a vector to get elements from.
 @return `true` if the values in `vec2` were successfully added to the vector, `false` otherwise 
 (out of memory error).
 */
#define ok_vec_push_all(vec, vec2) \
    (ok_vec_ensure_capacity(vec, (vec2)->count) ? \
    (memcpy((vec)->values + (vec)->count, (vec2)->values, sizeof(*(vec)->values) * (vec2)->count), \
    ((vec)->count += (vec2)->count)) : 0)

/**
 Inserts a value at the specified location in the vector. The element currently at that
 location, and all subsequent elements, are moved to the right by one position.

 @param vec Pointer to the vector.
 @param index `size_t` The index at which to insert the element. If the index is greater than or
 equal to number of elements in the vector, the value is added to the end, like #ok_vec_push().
 @param value The value to insert.
 */
#define ok_vec_insert_at(vec, index, value) \
    do { \
        if (ok_vec_ensure_capacity(vec, 1)) { \
            size_t _i1 = (index); \
            if (_i1 + 1 < (vec)->count) { \
                memmove((vec)->values + _i1 + 1, (vec)->values + _i1, \
                        ((vec)->count - _i1 - 1) * sizeof(*(vec)->values)); \
            } \
            (vec)->values[_i1] = (value); \
            (vec)->count++; \
        } \
    } while (0)

/**
 Removes an element at the specified location in the vector.

 @param vec Pointer to the vector.
 @param index `size_t` The index of the element to remove. If the index is greater than or
 equal to number of elements in the vector, the size of the vector is reduced by one.
 */
#define ok_vec_remove_at(vec, index) \
    do { \
        size_t _i2 = (index); \
        if (_i2 + 1 < (vec)->count) { \
            memmove((vec)->values + _i2, (vec)->values + _i2 + 1, \
                    ((vec)->count - _i2 - 1) * sizeof(*(vec)->values)); \
        } \
        if ((vec)->count > 0) { \
            (vec)->count--; \
        } \
    } while (0)

/**
 Removes the first element in the vector that equals the specified value.

 This function uses the equality operator `==` to test for equal values, which will fail to 
 compile if values are structs.

 @param vec Pointer to the vector.
 @param value The value to find and remove.
 */
#define ok_vec_remove(vec, value) \
    do { \
        for (size_t _i3 = 0; _i3 < (vec)->count; _i3++) { \
            if ((vec)->values[_i3] == (value)) { \
                ok_vec_remove_at(vec, _i3); \
                break; \
            } \
        } \
    } while (0)

/**
 Foreach macro that iterates over the values in the vector.

 The vector should not be modified during iteration. A call to #ok_vec_push()
 during iteration could cause a crash.

 The `break` and `continue` keywords are supported during iteration.

 Example:

 ok_vec_foreach(vec, char *value) {
     printf("Value: %s\n", value);
 }

 @param vec   Pointer to the vector.
 @param var   The value type and name.
 */
#define ok_vec_foreach(vec, var) \
    for (size_t _keep = 1, _i = 0, _len = (vec)->count; _keep && _i < _len; _keep = 1 - _keep, _i++) \
    for (var = *((vec)->values + _i); _keep; _keep = 1 - _keep)

/**
 Foreach macro that iterates over the values in the vector, in reverse order.

 The vector should not be modified during iteration. A call to #ok_vec_push()
 during iteration could cause as crash.

 The `break` and `continue` keywords are supported during iteration.

 Example:

 ok_vec_foreach_rev(vec, char *value) {
     printf("Value: %s\n", value);
 }

 @param vec   Pointer to the vector.
 @param var   The value type and name.
 */
#define ok_vec_foreach_rev(vec, var) \
    for (size_t _keep = 1, _i = 0, _len = (vec)->count; _keep && _i < _len; _keep = 1 - _keep, _i++) \
    for (var = *((vec)->values + (_len - _i - 1)); _keep; _keep = 1 - _keep)

/**
 Foreach macro that iterates over pointers to the values in the vector.

 The vector should not be modified during iteration. A call to #ok_vec_push()
 during iteration could cause as crash.

 The `break` and `continue` keywords are supported during iteration.

 Example:

 ok_vec_foreach_ptr(vec, char **value) {
     printf("Value: %s\n", *value);
 }

 @param vec   Pointer to the vector.
 @param var   The value type and name.
 */
#define ok_vec_foreach_ptr(vec, var) \
    for (size_t _keep = 1, _i = 0, _len = (vec)->count; _keep && _i < _len; _keep = 1 - _keep, _i++) \
    for (var = (vec)->values + _i; _keep; _keep = 1 - _keep)

/**
 Foreach macro that iterates over pointers to the values in the vector.

 The vector should not be modified during iteration. A call to #ok_vec_push()
 during iteration could cause as crash.

 The `break` and `continue` keywords are supported during iteration.

 Example:

 ok_vec_foreach_ptr_rev(vec, char **value) {
     printf("Value: %s\n", *value);
 }

 @param vec   Pointer to the vector.
 @param var   The value type and name.
 */
#define ok_vec_foreach_ptr_rev(vec, var) \
    for (size_t _keep = 1, _i = 0, _len = (vec)->count; _keep && _i < _len; _keep = 1 - _keep, _i++) \
    for (var = (vec)->values + (_len - _i - 1); _keep; _keep = 1 - _keep)

/**
 Applies a function to each element in a vector.

 @param vec Pointer to the vector.
 @param func The function.
 */
#define ok_vec_apply(vec, func) \
    for (size_t _i = 0, _len = (vec)->count; _i < _len; _i++) func(*((vec)->values + _i))

/**
 Applies a function to a pointer to each element in a vector.

 @param vec Pointer to the vector.
 @param func The function.
 */
#define ok_vec_apply_ptr(vec, func) \
    for (size_t _i = 0, _len = (vec)->count; _i < _len; _i++) func(((vec)->values + _i))

/**
 Sorts elements in a vector

 @param vec Pointer to the vector.
 @param compare_func Pointer to a function that compares two elements. Uses the same syntax as
 `qsort`.
 */
#define ok_vec_sort(vec, compare_func) \
    qsort((void *)(vec)->values, (vec)->count, sizeof(*((vec)->values)), compare_func)

/**
 Ensures that a vector has enough space for additional elements.

 @param vec Pointer to the vector.
 @param additional_count The number of addtional elements to make room for.
 @return `true` if successful, `false` otherwise (out of memory error).
 */
#define ok_vec_ensure_capacity(vec, additional_count) \
    (((vec)->count + (size_t)(additional_count) <= (vec)->capacity) ? true : \
    _ok_vec_realloc((void **)&(vec)->values, (vec)->count + (size_t)(additional_count), \
                    sizeof(*(vec)->values), &(vec)->capacity))

// MARK: Map

/**
 Declares a generic `ok_map` struct or typedef.

 For example, a map of string keys with `int` values can be declared as a typedef:

     typedef struct ok_map_of(const char *, int) my_map_t;

 or a struct:

     struct my_map_s ok_map_of(const char *, int);

 @tparam key_type   The key type.
 @tparam value_type The value type.

 @return Internal structure members in curly braces.
 */
#define ok_map_of(key_type, value_type) { \
    /* The `entry` struct has two purposes:
    1) Let the compiler determine member alignment and stride for the bucket array.
    2) Provide temp space for arguments and return values.
    The hash member must be first. */ \
    OK_MUTABLE struct { \
        ok_hash_t hash; \
        key_type k; \
        value_type v; \
    } entry; \
    OK_MUTABLE value_type *v_ptr; \
    struct _ok_map *m; \
    ok_hash_t (*key_hash_func)(key_type); \
}

/**
 Inits a map, automatically chooising hash and equals functions if possible. If not possible,
 a compile-time error occurs.
 
 When using C11, this function works when keys are integers, floats, or strings. When using C99,
 this function only works with const string keys (that is, `const char *`).
 
 To init a map with a custom key, use #ok_map_init_custom() instead.

 When finished using the map, the #ok_map_deinit() function must be called.

 @param map Pointer to the map.

 @return bool `true` if success, `false` otherwise (out of memory error).
 */
#define ok_map_init(map) \
    ok_map_init_with_capacity(map, 0)

/**
 Inits a map.

 When finished using the map, the #ok_map_deinit() function must be called.

 @param map         Pointer to the map.
 @param hash_func   The function to calculate the hash of the key. The signature of the function
                    is `ok_hash_t hash_func(key_type)`. For example, see #ok_const_str_hash().
 @param equals_func The function to determine if two keys are equal. The signature of the function
                    is `bool equals_func(void *, void *)`, where the parameters are pointers to the 
                    key. For example, see #ok_str_equals().
 
 @return bool `true` if success, `false` otherwise (out of memory error).
 */
#define ok_map_init_custom(map, hash_func, equals_func) \
    ok_map_init_custom_with_capacity(map, hash_func, equals_func, 0)

/**
 Inits a map with the specified initial capacity, automatically chooising hash and equals functions
 if possible. If not possible, a compile-time error occurs.

 When using C11, this function works when keys are integers, floats, or strings. When using C99,
 this function only works with const string keys (that is, `const char *`).

 To init a map with a custom key, use #ok_map_init_custom_with_capacity() instead.

 When finished using the map, the #ok_map_deinit() function must be called.

 @param map      Pointer to the map.
 @param capacity The initial capacity. If 0, the default capacity is used. The actual capacity will
                 be a power-of-two integer greater than or equal to the requested capacity.

 @return bool `true` if success, `false` otherwise (out of memory error).
 */
#define ok_map_init_with_capacity(map, capacity) \
    ok_map_init_custom_with_capacity(map, ok_default_hash((map)->entry.k), \
                                     ok_default_equals((map)->entry.k), capacity)

/**
 Inits a map.

 When finished using the map, the #ok_map_deinit() function must be called.

 @param map         Pointer to the map.
 @param hash_func   The function to calculate the hash of the key. The signature of the function
                    is `ok_hash_t hash_func(key_type)`. For example, see #ok_str_hash().
 @param equals_func The function to determine if two keys are equal. The signature of the function
                    is `bool equals_func(void *, void *)`, where the parameters are pointers to the
                    key. For example, see #ok_str_equals().
 @param capacity    The initial capacity. If 0, the default capacity is used. The actual capacity
                    may be a power-of-two integer greater than or equal to the requested capacity.

 @return bool `true` if success, `false` otherwise (out of memory error).
 */
#define ok_map_init_custom_with_capacity(map, hash_func, equals_func, capacity) ( \
    memset((map), 0, sizeof(*(map))), \
    (map)->key_hash_func = hash_func, \
    (((map)->m = _ok_map_create(capacity, equals_func, \
                                OK_OFFSETOF(&(map)->entry, &(map)->entry.k), \
                                OK_OFFSETOF(&(map)->entry, &(map)->entry.v), \
                                sizeof((map)->entry))) != NULL) \
)

/**
 Deinits the map. The map may be used again by calling #ok_map_init().

 @param map Pointer to the map.
 */
#define ok_map_deinit(map) \
    _ok_map_free((map)->m)

/**
 Gets the number of elements in the map.

 @param map Pointer to the map.

 @return size_t The number of elements in the map.
 */
#define ok_map_count(map) \
    _ok_map_count((map)->m)

/**
 Gets the capacity of a hash map. The capacity never shrinks.

 @param map Pointer to the map. If `NULL`, the default capacity is returned.

 @return size_t The capacity.
 */
#define ok_map_capacity(map) \
    _ok_map_capacity((map) ? (map)->m : NULL)

/**
 Puts a key-value pair into the map. If the key already exists in the map, it is replaced with
 the new value.

 @param map   Pointer to the map.
 @param key   The key.
 @param value The value.

 @return `true` if the operation was successful, `false` otherwise (out of memory).
 */
#define ok_map_put(map, key, value) ( \
    (map)->entry.k = (key), \
    (map)->entry.v = (value), \
    _ok_map_put(&(map)->m, &(map)->entry.k, sizeof((map)->entry.k), \
                (map)->key_hash_func((map)->entry.k), \
                &(map)->entry.v, sizeof((map)->entry.v)) \
)

/**
 Get a pointer to the value associated with a key, creating a new mapping if the key does not exist
 in the map.
 
 If the key didn't previously exist in the map, the value is undefined.
 
 Example:
 
 *    *ok_map_put_and_get_ptr(map, "jenny") = "867-5309";

 The returned pointer should be considered temporary. It may be invalid, and should not be used,
 after any modification to the map (like a call to #ok_map_put() or #ok_map_remove().)

 @param map Pointer to the map.
 @param key The key.

 @return A pointer to the value, or `NULL` for out-of-memory error.
 */
#define ok_map_put_and_get_ptr(map, key) ( \
    (map)->entry.k = (key), \
    _ok_map_put_and_get_ptr(&(map)->m, &(map)->entry.k, sizeof((map)->entry.k), \
                            (map)->key_hash_func((map)->entry.k), \
                            (void **)&(map)->v_ptr, sizeof((map)->entry.v)), \
    (map)->v_ptr \
)

/**
 Copies mappings from one map to another. The hash maps must have the same types, hash functions,
 amd equals functions.

 @param map      Pointer to the map.
 @param from_map Pointer to the map to copy mappings from.

 @return `true` if all of the mappings from `from_map` were copied to the map. If `false`, 
 (out of memory error), some of the mappings may have been copied before failure.
 */
#define ok_map_put_all(map, from_map) (\
    ((sizeof((map)->entry) == sizeof((from_map)->entry) && \
    (map)->key_hash_func == (from_map)->key_hash_func) ? \
    _ok_map_put_all(&(map)->m, (from_map)->m, sizeof((map)->entry.k), \
                    sizeof((map)->entry.v)) : \
    false) \
)

/**
 Gets a value from the map. If the key doesn't exist in the map, returns a zeroed-out value
 (`0`, `0.0`, `{0}`, `NULL`, etc.).

 @param map Pointer to the map.
 @param key The key.

 @return The value, or zero if the key doesn't exist in the map.
 */
#define ok_map_get(map, key) ( \
    (map)->entry.k = (key), \
    _ok_map_get((map)->m, &(map)->entry.k, (map)->key_hash_func((map)->entry.k), \
                (void *)&(map)->entry.v, sizeof((map)->entry.v)), \
    (map)->entry.v \
)

/**
 Get a pointer to the value associated with a key, or `NULL` if the key does not exist in the map.
 
 The returned pointer should be considered temporary. It may be invalid, and should not be used,
 after any modification to the map (like a call to #ok_map_put() or #ok_map_remove().)

 @param map Pointer to the map.
 @param key The key.

 @return A pointer to the value, or `NULL` if the key does not exist in the map.
 */
#define ok_map_get_ptr(map, key) ( \
    (map)->entry.k = (key), \
    _ok_map_get_ptr((map)->m, &(map)->entry.k, (map)->key_hash_func((map)->entry.k), \
                    (void **)&(map)->v_ptr), \
    (map)->v_ptr \
)

/**
 Checks if a key exists in the map.

 @param map Pointer to the map.
 @param key The key.

 @return bool `true` if the key exists, `false` otherwise.
 */
#define ok_map_contains(map, key) ( \
    (map)->entry.k = (key), \
    _ok_map_contains((map)->m, &(map)->entry.k, (map)->key_hash_func((map)->entry.k)) \
)

/**
 Removes a key from the map.

 @param map Pointer to the map.
 @param key The key to remove.

 @return `true` if the key was in the map (thus removed), `false` otherwise.
 */
#define ok_map_remove(map, key) ( \
    (map)->entry.k = (key), \
    _ok_map_remove((map)->m, &(map)->entry.k, (map)->key_hash_func((map)->entry.k)) \
)

/**
 Foreach macro that iterates over the keys and values in the map. The mappings are not returned in
 any particular order, and the order may change as the map is modified.
 
 The map should not be modified during iteration. A call to #ok_map_put() or #ok_map_remove()
 during iteration could cause as crash.
 
 The `break` and `continue` keywords are supported during iteration.
 
 Example:
 
     ok_map_foreach(map, const char *key, char *value) {
         printf("Name: %s  Phone: %s\n", key, value);
     }

 @param map       Pointer to the map.
 @param key_var   The key type and name.
 @param value_var The value type and name.
 */
#define ok_map_foreach(map, key_var, value_var) \
    for (size_t _keep = 1, _keep2 = 1, *_i = NULL; _keep && \
        ((_i = (size_t *)_ok_map_next((map)->m, _i, (void *)&(map)->entry.k, \
                                      sizeof((map)->entry.k), (void *)&(map)->entry.v, \
                                      sizeof((map)->entry.v))) != NULL); \
        _keep = 1 - _keep, _keep2 = 1 - _keep2) \
    for (key_var = (map)->entry.k; _keep && _keep2; _keep2 = 1 - _keep2) \
    for (value_var = (map)->entry.v; _keep; _keep = 1 - _keep)

// MARK: Concurrent queue

/**
 The default capacity of a queue. This is the minimum capacity; queues can grow in size.
 */
#define OK_QUEUE_DEFAULT_CAPACITY 16

/**
 A macro to initialize a queue statically.

     typedef struct ok_queue_of(int) my_queue_t;
     my_queue_t queue = OK_QUEUE_INIT;

 When finished using the queue, the #ok_queue_deinit() function must be called.
 */
#define OK_QUEUE_INIT { { NULL, NULL, NULL, false, false, OK_QUEUE_DEFAULT_CAPACITY }, NULL }

/**
 Declares a generic `ok_queue` struct or typedef.

 For example, a ok_queue with `int` values can be declared as a typedef:

     typedef struct ok_queue_of(int) my_queue_t;

 or a struct:

     struct my_queue_s ok_queue_of(int);

 @tparam value_type The value type.

 @return Internal structure members in curly braces.
 */
#define ok_queue_of(value_type) { \
    struct _ok_queue q; \
    value_type *v; \
}

/**
 Inits a queue with the default minimum capacity.

 To init a queue with a custom minimum capacity, use #ok_queue_init_with_capacity() instead.

 This function is not thread safe. If two threads attempt to init a queue at the same time, the
 result is undefined.

 When finished using the queue, the #ok_queue_deinit() function must be called.

 @tparam queue Pointer to the queue.
 */
#define ok_queue_init(queue) \
    ok_queue_init_with_capacity(queue, OK_QUEUE_DEFAULT_CAPACITY)

/**
 Inits a queue with a custom minimum capacity. The queue may grow in size larger than the specified
 capacity.

 This function is not thread safe. If two threads attempt to init a queue at the same time, the
 result is undefined.

 When finished using the queue, the #ok_queue_deinit() function must be called.

 @tparam queue Pointer to the queue.
 @tparam capacity The minimum number of values the queue can hold.
 */
#define ok_queue_init_with_capacity(queue, capacity) \
    _ok_queue_init(&(queue)->q, sizeof(*(queue)->v), capacity)

/**
 Deinits the queue. The queue may be used again by calling #ok_queue_init().

 @tparam queue Pointer to the queue.
 */
#define ok_queue_deinit(queue) \
    ok_queue_deinit_with_deallocator(queue, NULL)

/**
 Deinits the queue and deallocates its remaining values. Each remaining value is
 removed from the queue and sent as a param to the deallocator function.

 The queue may be used again by calling #ok_queue_init().

 @tparam queue Pointer to the queue.
 @tparam deallocator The deallocator function, declared as `void (*deallocator)(void *)`.
 */
#define ok_queue_deinit_with_deallocator(queue, deallocator) \
    _ok_queue_deinit(&(queue)->q, sizeof(*(queue)->v), (deallocator))

/**
 Adds a value to the back of the queue.

 @tparam queue Pointer to the queue.
 @tparam value The value to add. Must be an addressable rvalue.
 */
#define ok_queue_push(queue, value) do { \
    ok_static_assert(ok_types_compatible(*(queue)->v, value), "Incompatible types"); \
    _ok_queue_push(&(queue)->q, sizeof(*(queue)->v), &(value)); \
} while (0)

/**
 Attempts to remove a value from the front of the queue.

 @tparam queue Pointer to the queue.
 @tparam value_ptr The address to store the removed value.
 @return true on success, false if the queue is empty.
 */
#define ok_queue_pop(queue, value_ptr) (\
    sizeof(char[ok_types_compatible(*(queue)->v, *(value_ptr)) ? 1 : -1]) && /* Type check */ \
    _ok_queue_pop(&(queue)->q, sizeof(*(queue)->v), (value_ptr)) \
)

// MARK: Declarations: Hash functions

/// The hash type, which is returned from hash functions.
typedef uint32_t ok_hash_t;

/// Gets the default hash function for the specified key type. Uses _Generic, so it requires C11.
#ifndef ok_default_hash
#  if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#    define ok_default_hash(key) _Generic(key, \
       uint8_t      : ok_uint8_hash, \
       int8_t       : ok_int8_hash, \
       uint16_t     : ok_uint16_hash, \
       int16_t      : ok_int16_hash, \
       uint32_t     : ok_uint32_hash, \
       int32_t      : ok_int32_hash, \
       uint64_t     : ok_uint64_hash, \
       int64_t      : ok_int64_hash, \
       float        : ok_float_hash, \
       double       : ok_double_hash, \
       char *       : ok_str_hash, \
       const char * : ok_const_str_hash, \
       void *       : ok_ptr_hash, \
       const void * : ok_const_ptr_hash)
#  else
// Force compile error if type is not 'char *`
#    define ok_default_hash(key) (ok_static_assert(sizeof(_ok_is_char(*key)) == sizeof(bool) && \
                                                   sizeof(*key) == sizeof(char), \
                                                   "Only works with `char *` type"), \
                                                   ok_const_str_hash)
#  endif
#endif

static inline bool _ok_is_char(char);

/// Gets the hash for a uint8_t.
OK_LIB_API ok_hash_t ok_uint8_hash(uint8_t key);

/// Gets the hash for a int8_t.
OK_LIB_API ok_hash_t ok_int8_hash(int8_t key);

/// Gets the hash for a uint16_t.
OK_LIB_API ok_hash_t ok_uint16_hash(uint16_t key);

/// Gets the hash for a int16_t.
OK_LIB_API ok_hash_t ok_int16_hash(int16_t key);

/// Gets the hash for a uint32_t.
OK_LIB_API ok_hash_t ok_uint32_hash(uint32_t key);

/// Gets the hash for a int32_t.
OK_LIB_API ok_hash_t ok_int32_hash(int32_t key);

/// Gets the hash for a uint64_t.
OK_LIB_API ok_hash_t ok_uint64_hash(uint64_t key);

/// Gets the hash for a int64_t.
OK_LIB_API ok_hash_t ok_int64_hash(int64_t key);

/// Gets the hash for a float.
OK_LIB_API ok_hash_t ok_float_hash(float key);

/// Gets the hash for a double.
OK_LIB_API ok_hash_t ok_double_hash(double key);

/// Gets the hash for a string.
OK_LIB_API ok_hash_t ok_str_hash(char *key);

/// Gets the hash for a string.
OK_LIB_API ok_hash_t ok_const_str_hash(const char *key);

/// Combines two hashes into one.
OK_LIB_API ok_hash_t ok_hash_combine(ok_hash_t hash_a, ok_hash_t hash_b);

// MARK: Declarations: Equals functions

/// Gets the default equals function for the specified key type. Uses _Generic, so it requires C11.
#ifndef ok_default_equals
#  if defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 201112L)
#    define ok_default_equals(key) _Generic(key, \
       uint8_t      : ok_8bit_equals, \
       int8_t       : ok_8bit_equals, \
       uint16_t     : ok_16bit_equals, \
       int16_t      : ok_16bit_equals, \
       uint32_t     : ok_32bit_equals, \
       int32_t      : ok_32bit_equals, \
       uint64_t     : ok_64bit_equals, \
       int64_t      : ok_64bit_equals, \
       float        : ok_32bit_equals, \
       double       : ok_64bit_equals, \
       char *       : ok_str_equals, \
       const char * : ok_str_equals)
#  else
#    define ok_default_equals(key) ok_str_equals
#  endif
#endif

/// Checks if two 8-bit values are equal.
OK_LIB_API bool ok_8bit_equals(const void *v1, const void *v2);

/// Checks if two 16-bit values are equal.
OK_LIB_API bool ok_16bit_equals(const void *v1, const void *v2);

/// Checks if two 32-bit values are equal.
OK_LIB_API bool ok_32bit_equals(const void *v1, const void *v2);

/// Checks if two 64-bit values are equal.
OK_LIB_API bool ok_64bit_equals(const void *v1, const void *v2);

/// Checks if two strings are equal.
OK_LIB_API bool ok_str_equals(const void *a, const void *b);

// MARK: Declarations: Private functions

// @cond private

#define OK_PTR_INC(ptr, offset) ((uint8_t *)(ptr) + (offset))
#define OK_OFFSETOF(base_ptr, ptr) ((size_t)((uint8_t *)(ptr) - (uint8_t *)(base_ptr)))

struct _ok_map;
struct _ok_queue_block;
struct _ok_queue;

OK_LIB_API bool _ok_vec_realloc(void **values, size_t min_capacity, size_t element_size,
                                size_t *capacity);

OK_LIB_API struct _ok_map *_ok_map_create(size_t initial_capacity,
                                          bool (*key_equals_func)(const void *key1,
                                                                  const void *key2),
                                          size_t key_offset, size_t value_offset,
                                          size_t bucket_stride);

OK_LIB_API void _ok_map_free(struct _ok_map *map);

OK_LIB_API size_t _ok_map_count(const struct _ok_map *map);

OK_LIB_API size_t _ok_map_capacity(const struct _ok_map *map);

OK_LIB_API bool _ok_map_contains(const struct _ok_map *map, const void *key,
                                  ok_hash_t key_hash);

OK_LIB_API bool _ok_map_put(struct _ok_map **map, const void *key,
                            size_t key_size, ok_hash_t key_hash,
                            const void *value, size_t value_size);

OK_LIB_API void _ok_map_put_and_get_ptr(struct _ok_map **map, const void *key,
                                        size_t key_size, ok_hash_t key_hash,
                                        void **value_ptr, size_t value_size);

OK_LIB_API bool _ok_map_put_all(struct _ok_map **map,
                                const struct _ok_map *from_map,
                                size_t key_size, size_t value_size);

OK_LIB_API void _ok_map_get(const struct _ok_map *map, const void *key,
                            ok_hash_t key_hash, void *value, size_t value_size);

OK_LIB_API void _ok_map_get_ptr(const struct _ok_map *map, const void *key,
                                ok_hash_t key_hash, void **value_ptr);

OK_LIB_API bool _ok_map_remove(struct _ok_map *map, const void *key,
                               ok_hash_t key_hash);

OK_LIB_API void *_ok_map_next(const struct _ok_map *map, void *iterator, void *key,
                              size_t key_size, void *value, size_t value_size);

OK_LIB_API struct _ok_queue_block *_ok_queue_new_block(const struct _ok_queue *queue,
                                                       size_t value_size);

OK_LIB_API void _ok_queue_free_block(struct _ok_queue_block *block);

OK_LIB_API struct _ok_queue_block *_ok_queue_get_free_block(struct _ok_queue *queue,
                                                            size_t value_size);

OK_LIB_API void _ok_queue_release_free_block(struct _ok_queue *queue,
                                             struct _ok_queue_block *block);

OK_LIB_API void _ok_queue_set_value(void *values, size_t value_size, size_t index, void *value);

OK_LIB_API void _ok_queue_get_value(void *values, size_t value_size, size_t index, void *value);

OK_LIB_API bool _ok_queue_pop(struct _ok_queue *queue, size_t value_size, void *value);

OK_LIB_API void _ok_queue_push(struct _ok_queue *queue, size_t value_size, void *value);

OK_LIB_API void _ok_queue_init(struct _ok_queue *queue, size_t value_size, size_t capacity);

OK_LIB_API void _ok_queue_deinit(struct _ok_queue *queue, size_t value_size,
                                 void (*deallocator)(void *));

// MARK: Implementation: Hash functions

#ifdef OK_LIB_DEFINE

#if defined(__GNUC__)
#  pragma GCC diagnostic push
#  pragma GCC diagnostic ignored "-Wunused-function"
#elif defined (_MSC_VER)
#  pragma warning(push)
#  pragma warning(disable:4505)
#endif

// Hash functions from Wang http://www.cris.com/~Ttwang/tech/inthash.htm
// and Jenkins http://www.burtleburtle.net/bob/hash/doobs.html

OK_LIB_API ok_hash_t ok_uint8_hash(uint8_t key) {
    return ok_uint32_hash(key);
}

OK_LIB_API ok_hash_t ok_int8_hash(int8_t key) {
    return ok_uint32_hash((uint32_t)key);
}

OK_LIB_API ok_hash_t ok_uint16_hash(uint16_t key) {
    return ok_uint32_hash(key);
}

OK_LIB_API ok_hash_t ok_int16_hash(int16_t key) {
    return ok_uint32_hash((uint32_t)key);
}

OK_LIB_API ok_hash_t ok_uint32_hash(uint32_t key) {
    key += ~(key << 16);
    key ^=  (key >> 5);
    key +=  (key << 3);
    key ^=  (key >> 13);
    key += ~(key << 9);
    key ^=  (key >> 17);
    return key;
}

OK_LIB_API ok_hash_t ok_int32_hash(int32_t key) {
    return ok_uint32_hash((uint32_t)key);
}

OK_LIB_API ok_hash_t ok_float_hash(float key) {
    uint32_t int_key;
    ok_static_assert(sizeof(key) == sizeof(int_key), "float must be be 32-bit");
    memcpy(&int_key, &key, sizeof(int_key));
    return ok_uint32_hash(int_key);
}

OK_LIB_API ok_hash_t ok_uint64_hash(uint64_t key) {
    key += ~(key << 34);
    key ^=  (key >> 29);
    key += ~(key << 11);
    key ^=  (key >> 14);
    key += ~(key <<  7);
    key ^=  (key >> 28);
    key += ~(key << 26);
    return (ok_hash_t)key;
}

OK_LIB_API ok_hash_t ok_int64_hash(int64_t key) {
    return ok_uint64_hash((uint64_t)key);
}

OK_LIB_API ok_hash_t ok_double_hash(double key) {
    uint64_t int_key;
    ok_static_assert(sizeof(key) == sizeof(int_key), "double must be 64-bit");
    memcpy(&int_key, &key, sizeof(int_key));
    return ok_uint64_hash(int_key);
}

OK_LIB_API ok_hash_t ok_const_str_hash(const char *key) {
    ok_hash_t hash = 0;
    char ch;
    while ((ch = *key++) != 0) {
        hash += (ok_hash_t)ch;
        hash += (hash << 10);
        hash ^= (hash >> 6);
    }
    hash += (hash << 3);
    hash ^= (hash >> 11);
    hash += (hash << 15);
    return hash;
}

OK_LIB_API ok_hash_t ok_str_hash(char *key) {
    return ok_const_str_hash(key);
}

OK_LIB_API ok_hash_t ok_const_ptr_hash(const void *key) {
#if UINTPTR_MAX == UINT32_MAX
    uint32_t int_key;
    memcpy(&int_key, &key, sizeof(int_key));
    return ok_uint32_hash(int_key);
#elif UINTPTR_MAX == UINT64_MAX
    uint64_t int_key;
    memcpy(&int_key, &key, sizeof(int_key));
    return ok_uint64_hash(int_key);
#else
    ok_static_assert(false, "Unknown pointer size");
    return 0;
#endif
}

OK_LIB_API ok_hash_t ok_ptr_hash(void *key) {
    return ok_const_ptr_hash(key);
}

OK_LIB_API ok_hash_t ok_hash_combine(ok_hash_t hash_a, ok_hash_t hash_b) {
    return hash_a ^ (hash_b + 0x9e3779b9 + (hash_a << 6) + (hash_a >> 2));
}

// MARK: Implementation: Equals functions

OK_LIB_API bool ok_8bit_equals(const void *v1, const void *v2) {
    uint8_t a = *(const uint8_t *)v1;
    uint8_t b = *(const uint8_t *)v2;
    return a == b;
}

OK_LIB_API bool ok_16bit_equals(const void *v1, const void *v2) {
    uint16_t a = *(const uint16_t *)v1;
    uint16_t b = *(const uint16_t *)v2;
    return a == b;
}

OK_LIB_API bool ok_32bit_equals(const void *v1, const void *v2) {
    uint32_t a = *(const uint32_t *)v1;
    uint32_t b = *(const uint32_t *)v2;
    return a == b;
}

OK_LIB_API bool ok_64bit_equals(const void *v1, const void *v2) {
    uint64_t a = *(const uint64_t *)v1;
    uint64_t b = *(const uint64_t *)v2;
    return a == b;
}

OK_LIB_API bool ok_str_equals(const void *a, const void *b) {
    const char *str1 = *(const char * const *)a;
    const char *str2 = *(const char * const *)b;

    return strcmp(str1, str2) == 0;
}

OK_LIB_API bool ok_ptr_equals(const void *v1, const void *v2) {
    const void *a = *(const void * const *)v1;
    const void *b = *(const void * const *)v2;
    return a == b;
}

// MARK: Implementation: Private vector functions

OK_LIB_API bool _ok_vec_realloc(void **values, size_t min_capacity,
                                size_t element_size, size_t *capacity) {
    const size_t capacity_2 = *capacity << 1;
    min_capacity = min_capacity < 8 ? 8 : min_capacity;
    const size_t new_capacity = capacity_2 > min_capacity ? capacity_2 : min_capacity;
    void *new_values = realloc(*values, element_size * new_capacity);
    if (new_values) {
        *values = new_values;
        *capacity = new_capacity;
        return true;
    } else {
        return false;
    }
}

// MARK: Implementation: Private map functions

/*
 Hashtable implementation notes:
 1) Open addressing
 2) Linear probing
 3) Cleanup on deletion (no lazy deletion)

 Only the lower 31 bits of the hash are used. The upper bit is the "occupied" flag.

 References:
 * https://en.wikipedia.org/wiki/Open_addressing
 * http://research.cs.vt.edu/AVresearch/hashing/index.php
 */

static const ok_hash_t OK_MAP_OCCUPIED_FLAG = 0x80000000;
static const size_t OK_MAP_MIN_CAPACITY = 32;
static const float OK_MAP_DEFAULT_MAX_LOAD = 0.75f;

struct _ok_map {
    void *buckets;

    size_t key_offset;
    size_t value_offset;
    size_t bucket_stride;

    size_t capacity_n;
    size_t capacity_mask;
    size_t max_count;
    size_t count; // The count is the only member that is mutated after _ok_map_init()

    bool (*key_equals_func)(const void *key1, const void *key2);

    float max_load_factor;
};

static struct _ok_map *_ok_map_init(struct _ok_map *map, size_t initial_capacity) {
    map->count = 0;
    if (initial_capacity < OK_MAP_MIN_CAPACITY) {
        initial_capacity = OK_MAP_MIN_CAPACITY;
    }
    size_t capacity_n = 0;
    while ((1u << capacity_n) < initial_capacity) {
        capacity_n++;
    }
    size_t capacity = (1u << capacity_n);

    map->buckets = calloc(capacity, map->bucket_stride);
    if (map->buckets) {
        map->capacity_n = capacity_n;
        map->capacity_mask = capacity - 1;
        map->max_count = (size_t)(capacity * map->max_load_factor);
        // Make sure there is always at least one free entry (for _ok_map_find_entry)
        if (map->max_count >= capacity) {
            map->max_count = capacity - 1;
        }
    } else {
        free(map);
        map = NULL;
    }
    return map;
}

static struct _ok_map *_ok_map_copy(const struct _ok_map *from_map,
                                    size_t initial_capacity,
                                    size_t key_size, size_t value_size) {
    struct _ok_map *map = (struct _ok_map *)calloc(1, sizeof(struct _ok_map));
    if (map) {
        map->key_equals_func = from_map->key_equals_func;
        map->key_offset = from_map->key_offset;
        map->value_offset = from_map->value_offset;
        map->bucket_stride = from_map->bucket_stride;
        map->max_load_factor = from_map->max_load_factor;
        map = _ok_map_init(map, initial_capacity);
        if (map) {
            bool success = _ok_map_put_all(&map, from_map, key_size, value_size);
            if (!success) {
                _ok_map_free(map);
                map = NULL;
            }
        }
    }

    return map;
}

static void *_ok_map_find_entry(const struct _ok_map *map, const void *key,
                                ok_hash_t key_hash, void **empty_entry) {
    ok_hash_t hash = key_hash | OK_MAP_OCCUPIED_FLAG;
    size_t bucket_index = (size_t)(hash & map->capacity_mask);
    while (true) {
        void *bucket = OK_PTR_INC(map->buckets, bucket_index * map->bucket_stride);
        ok_hash_t flags_hash = *(ok_hash_t *)(bucket);
        if (hash == flags_hash &&
            map->key_equals_func(OK_PTR_INC(bucket, map->key_offset), key)) {
            return bucket;
        } else if ((flags_hash & OK_MAP_OCCUPIED_FLAG) == 0) {
            if (empty_entry) {
                *empty_entry = bucket;
            }
            return NULL;
        }
        bucket_index = (bucket_index + 1) & map->capacity_mask; // Linear probing.
    }
}

static void *_ok_map_find_or_put_entry(struct _ok_map **map, const void *key,
                                       size_t key_size, ok_hash_t key_hash, size_t value_size) {
    void *new_entry = NULL;
    void *entry = _ok_map_find_entry(*map, key, key_hash, &new_entry);
    if (!entry) {
        // Grow
        if ((*map)->count >= (*map)->max_count) {
            struct _ok_map *new_map = _ok_map_copy(*map, (1u << ((*map)->capacity_n + 1)),
                                                     key_size, value_size);
            if (!new_map) {
                return NULL;
            }
            _ok_map_free(*map);
            *map = new_map;
            new_entry = NULL;
            _ok_map_find_entry(*map, key, key_hash, &new_entry);
        }
        if (new_entry) {
            entry = new_entry;
            key_hash |= OK_MAP_OCCUPIED_FLAG;
            memcpy(entry, &key_hash, sizeof(ok_hash_t));
            memcpy(OK_PTR_INC(entry, (*map)->key_offset), key, key_size);
            (*map)->count++;
        }
    }
    return entry;
}

OK_LIB_API struct _ok_map *_ok_map_create(size_t initial_capacity,
                                          bool (*key_equals_func)(const void *key1,
                                                                  const void *key2),
                                          size_t key_offset, size_t value_offset,
                                          size_t bucket_stride) {
    struct _ok_map *map = (struct _ok_map *)calloc(1, sizeof(struct _ok_map));
    if (map) {
        map->key_equals_func = key_equals_func;
        map->key_offset = key_offset;
        map->value_offset = value_offset;
        map->bucket_stride = bucket_stride;
        map->max_load_factor = OK_MAP_DEFAULT_MAX_LOAD;
        map = _ok_map_init(map, initial_capacity);
    }
    return map;
}

OK_LIB_API void _ok_map_free(struct _ok_map *map) {
    if (map) {
        free(map->buckets);
        free(map);
    }
}

OK_LIB_API size_t _ok_map_count(const struct _ok_map *map) {
    return map->count;
}

OK_LIB_API size_t _ok_map_capacity(const struct _ok_map *map) {
    return map ? (1u << map->capacity_n) : OK_MAP_MIN_CAPACITY;
}

OK_LIB_API bool _ok_map_contains(const struct _ok_map *map, const void *key,
                                 ok_hash_t key_hash) {
    return (_ok_map_find_entry(map, key, key_hash, NULL) != NULL);
}

OK_LIB_API bool _ok_map_put(struct _ok_map **map, const void *key,
                            size_t key_size, ok_hash_t key_hash,
                            const void *value, size_t value_size) {
    void *entry = _ok_map_find_or_put_entry(map, key, key_size, key_hash, value_size);
    if (entry) {
        memcpy(OK_PTR_INC(entry, (*map)->value_offset), value, value_size);
        return true;
    } else {
        return false;
    }
}

OK_LIB_API void _ok_map_put_and_get_ptr(struct _ok_map **map, const void *key,
                                        size_t key_size, ok_hash_t key_hash,
                                        void **value_ptr, size_t value_size) {
    void *entry = _ok_map_find_or_put_entry(map, key, key_size, key_hash, value_size);
    if (entry) {
        *value_ptr = OK_PTR_INC(entry, (*map)->value_offset);
    } else {
        *value_ptr = NULL;
    }
}

OK_LIB_API bool _ok_map_put_all(struct _ok_map **map,
                                const struct _ok_map *from_map,
                                size_t key_size, size_t value_size) {
    if ((*map)->key_equals_func != from_map->key_equals_func) {
        return false;
    }

    void *iterator = from_map->buckets;
    void *end = OK_PTR_INC(from_map->buckets, (from_map->bucket_stride << from_map->capacity_n));
    while (iterator < end) {
        ok_hash_t flags_hash = *(ok_hash_t *)(iterator);
        if (flags_hash & OK_MAP_OCCUPIED_FLAG) {
            void *key = OK_PTR_INC(iterator, from_map->key_offset);
            void *value = OK_PTR_INC(iterator, from_map->value_offset);
            bool success = _ok_map_put(map, key, key_size, flags_hash, value, value_size);
            if (!success) {
                return false;
            }
        }
        iterator = OK_PTR_INC(iterator, from_map->bucket_stride);
    }
    return true;
}

OK_LIB_API void _ok_map_get(const struct _ok_map *map, const void *key,
                            ok_hash_t key_hash, void *value, size_t value_size) {
    void *entry = _ok_map_find_entry(map, key, key_hash, NULL);
    if (entry) {
        memcpy(value, OK_PTR_INC(entry, map->value_offset), value_size);
    } else {
        memset(value, 0, value_size);
    }
}

OK_LIB_API void _ok_map_get_ptr(const struct _ok_map *map, const void *key,
                                ok_hash_t key_hash, void **value_ptr) {
    void *entry = _ok_map_find_entry(map, key, key_hash, NULL);
    if (entry) {
        *value_ptr = OK_PTR_INC(entry, map->value_offset);
    } else {
        *value_ptr = NULL;
    }
}

OK_LIB_API void *_ok_map_next(const struct _ok_map *map, void *iterator, void *key,
                              size_t key_size, void *value, size_t value_size) {
    if (map->count == 0) {
        return NULL;
    }
    void *begin = map->buckets;
    void *end = OK_PTR_INC(map->buckets, (map->bucket_stride << map->capacity_n));
    if (!iterator) {
        iterator = map->buckets;
    }
    while (iterator >= begin && iterator < end) {
        ok_hash_t flags_hash = *(ok_hash_t *)(iterator);
        void *next_iterator = OK_PTR_INC(iterator, map->bucket_stride);
        if (flags_hash & OK_MAP_OCCUPIED_FLAG) {
            if (key) {
                memcpy(key, OK_PTR_INC(iterator, map->key_offset), key_size);
            }
            if (value) {
                memcpy(value, OK_PTR_INC(iterator, map->value_offset), value_size);
            }
            return next_iterator;
        }
        iterator = next_iterator;
    }
    return NULL;
}

OK_LIB_API bool _ok_map_remove(struct _ok_map *map, const void *key, ok_hash_t key_hash) {
    void *removed_entry = _ok_map_find_entry(map, key, key_hash, NULL);
    if (!removed_entry) {
        return false;
    }
    memset(removed_entry, 0, sizeof(ok_hash_t));
    map->count--;

    size_t i = OK_OFFSETOF(map->buckets, removed_entry) / map->bucket_stride;
    size_t j = i;

    // NOTE: This only works with linear probing
    const size_t mask = (1u << map->capacity_n) - 1u;
    while (true) {
        j = (j + 1) & mask;
        void *entry = OK_PTR_INC(map->buckets, j * map->bucket_stride);

        ok_hash_t flags_hash = *(ok_hash_t *)(entry);
        if ((flags_hash & OK_MAP_OCCUPIED_FLAG) == 0) {
            break;
        }
        size_t k = flags_hash & mask;
        if ((i <= j) ? ((k <= i) || (k > j)) : ((k <= i) && (k > j))) {
            memcpy(removed_entry, entry, map->bucket_stride);
            i = j;
            removed_entry = entry;
            memset(removed_entry, 0, sizeof(ok_hash_t));
        }
    }
    return true;
}

// MARK: Implementation: Private queue functions

/*
 Queue
 - Multi-consumer, multi-producer concurrent queue.
 - If the number of elements is never greater than `capacity`, then no allocation occurs
 (after the initial allocation in ok_queue_init)
 - No lock contention if there is only one producer thread and one consumer thread.
 - Can be initialized statically with OK_QUEUE_INIT

 Based off of the two-lock queue from Michael and Scott:
 https://www.research.ibm.com/people/m/michael/podc-1996.pdf
 Using blocks of elements instead of nodes, and keeping 1 free block.

 Use stdatomic.h if available (GCC 4.9, clang 3.7), otherwise implement the bare minimum needed
 for the concurrent queue.

 Notes on stdatomic.h
 - GCC: stdatomic.h appears incompatible with C++'s <atomic>
 - Clang 3.6 has a bug where it will use GCC's stdatomic.h by mistake, causing a build error.
 */

#if !defined(OK_LIB_USE_STDATOMIC) && !defined(__EMSCRIPTEN__)
#  if defined(__clang__) && (__clang_major__ > 3 || (__clang_major__ == 3 && __clang_minor__ >= 7))
#    define OK_LIB_USE_STDATOMIC
#  elif defined(__GNUC__) && !defined(__cplusplus) && (__GNUC__ > 4 || (__GNUC__ == 4 && __GNUC_MINOR__ >= 9))
#    define OK_LIB_USE_STDATOMIC
#  endif
#endif
#if defined(OK_LIB_USE_STDATOMIC)
#  include <stdatomic.h>
#  define OK_LOCK_TYPE _Atomic(bool)
#  define OK_TRYLOCK(lock) (atomic_exchange_explicit((lock), true, memory_order_acquire) == false)
#  define OK_LOCK(lock) do { } while (!OK_TRYLOCK(lock))
#  define OK_UNLOCK(lock) atomic_store_explicit((lock), false, memory_order_release)
#elif defined(__EMSCRIPTEN__) // Assume single-threaded operation
#  if !defined(__STDC_VERSION__) || (__STDC_VERSION__ < 201112L)
#    pragma clang diagnostic push
#    pragma clang diagnostic ignored "-Wreserved-id-macro"
#    define _Atomic(T) T
#    pragma clang diagnostic pop
#  endif
#  define atomic_load(object) *(object)
#  define atomic_store(object, value) *(object) = value
#  define atomic_compare_exchange_strong(object, expected, desired) \
     (*(object) == *(expected) ? ((*(object) = desired), true) : false)
#  define OK_LOCK_TYPE _Atomic(bool)
#  define OK_TRYLOCK(lock) (*(lock) == false ? (*(lock) = true) : false)
#  define OK_LOCK(lock) do { } while (!OK_TRYLOCK(lock))
#  define OK_UNLOCK(lock) (*(lock) = false)
#elif defined(_MSC_VER)
#  define WIN32_LEAN_AND_MEAN
#  pragma warning(push, 0)
#  include <windows.h>
#  pragma warning(pop)
#  define _Atomic(T) T volatile
#  define atomic_load(object) (MemoryBarrier(), *(object))
#  define atomic_store(object, desired) do { *(object) = (desired); MemoryBarrier(); } while (0)
#  define atomic_compare_exchange_strong(object, expected, desired) \
     (InterlockedCompareExchangePointer((PVOID volatile *)(object), (desired), *(expected)) \
       == *(expected))
#  define OK_LOCK_TYPE LONG volatile
#  define OK_TRYLOCK(lock) (InterlockedExchange((lock), 1) == 0)
#  define OK_LOCK(lock) do { } while (!OK_TRYLOCK(lock))
#  define OK_UNLOCK(lock) atomic_store((lock), 0)
#elif defined(__GCC_HAVE_SYNC_COMPARE_AND_SWAP_4)
#  define _Atomic(T) T volatile
#  define atomic_load(object) __atomic_load_n((object), __ATOMIC_SEQ_CST)
#  define atomic_store(object, desired) __atomic_store_n((object), (desired), __ATOMIC_SEQ_CST)
#  define atomic_compare_exchange_strong(object, expected, desired) \
     __atomic_compare_exchange_n((object), (expected), (desired), 0, __ATOMIC_SEQ_CST, \
                                 __ATOMIC_SEQ_CST)
#  define OK_LOCK_TYPE bool volatile
#  define OK_TRYLOCK(lock) (__atomic_exchange_n((lock), true, __ATOMIC_ACQUIRE) == false)
#  define OK_LOCK(lock) do { } while (!OK_TRYLOCK(lock))
#  define OK_UNLOCK(lock) (void)__atomic_exchange_n((lock), false, __ATOMIC_RELEASE)
#else
#  error stdatomic.h required
#endif
#if defined(_MSC_VER)
#  define OK_ALIGNAS(N) __declspec(align(N))
#elif defined(__GNUC__)
#  define OK_ALIGNAS(N) __attribute__((aligned(N)))
#else
#  define OK_ALIGNAS(N) _Alignas(N)
#endif

#define OK_CACHELINE_SIZE 64

struct _ok_queue_block {
    OK_ALIGNAS(OK_CACHELINE_SIZE) void *values;
    OK_ALIGNAS(OK_CACHELINE_SIZE) struct _ok_queue_block *next;
    OK_ALIGNAS(OK_CACHELINE_SIZE) size_t head_index;
    OK_ALIGNAS(OK_CACHELINE_SIZE) _Atomic(size_t) tail_index;
};

struct _ok_queue {
    OK_ALIGNAS(OK_CACHELINE_SIZE) _Atomic(struct _ok_queue_block *) head_block;
    OK_ALIGNAS(OK_CACHELINE_SIZE) _Atomic(struct _ok_queue_block *) tail_block;
    OK_ALIGNAS(OK_CACHELINE_SIZE) _Atomic(struct _ok_queue_block *) free_block;
    OK_ALIGNAS(OK_CACHELINE_SIZE) OK_LOCK_TYPE head_lock;
    OK_ALIGNAS(OK_CACHELINE_SIZE) OK_LOCK_TYPE tail_lock;
    OK_ALIGNAS(OK_CACHELINE_SIZE) size_t block_capacity;
};

OK_LIB_API struct _ok_queue_block *_ok_queue_new_block(const struct _ok_queue *queue,
                                                       size_t value_size) {
    // Could this be reduced to one malloc, and still be aligned properly?
    struct _ok_queue_block *block;
    block = (struct _ok_queue_block *)malloc(sizeof(struct _ok_queue_block));
    block->values = malloc(value_size * queue->block_capacity);
    return block;
}

OK_LIB_API void _ok_queue_free_block(struct _ok_queue_block *block) {
    if (block) {
        free(block->values);
        free(block);
    }
}

OK_LIB_API struct _ok_queue_block *_ok_queue_get_free_block(struct _ok_queue *queue,
                                                            size_t value_size) {
    struct _ok_queue_block *free_block = atomic_load(&queue->free_block);
    if (free_block) {
        if (!atomic_compare_exchange_strong(&queue->free_block, &free_block, NULL)) {
            free_block = NULL;
        }
    }
    if (!free_block) {
        free_block = _ok_queue_new_block(queue, value_size);
    }
    return free_block;
}


OK_LIB_API void _ok_queue_release_free_block(struct _ok_queue *queue,
                                             struct _ok_queue_block *block) {
    // Currently keeping only one free block here.
    // It would be trivial to keep all of them (this could be an option, like block_capacity)
    struct _ok_queue_block *null_block = NULL;
    if (atomic_compare_exchange_strong(&queue->free_block, &null_block, block)) {
        block = NULL;
    }
    if (block) {
        _ok_queue_free_block(block);
    }
}

OK_LIB_API void _ok_queue_set_value(void *values, size_t value_size, size_t index, void *value) {
    memcpy(OK_PTR_INC(values, index * value_size), value, value_size);
}

OK_LIB_API void _ok_queue_get_value(void *values, size_t value_size, size_t index, void *value) {
    if (value) {
        memcpy(value, OK_PTR_INC(values, index * value_size), value_size);
    }
}

OK_LIB_API bool _ok_queue_pop(struct _ok_queue *queue, size_t value_size, void *value) {
    OK_LOCK(&queue->head_lock);
    struct _ok_queue_block *head_block = atomic_load(&queue->head_block);
    if (head_block == NULL || head_block->head_index == atomic_load(&head_block->tail_index)) {
        OK_UNLOCK(&queue->head_lock);
        return false;
    } else if (head_block->head_index == queue->block_capacity - 1) {
        struct _ok_queue_block *curr_block = head_block;
        struct _ok_queue_block *next_block = curr_block->next;
        _ok_queue_get_value(next_block->values, value_size, 0, value);
        atomic_store(&queue->head_block, next_block);
        head_block->head_index = 0;
        OK_UNLOCK(&queue->head_lock);
        _ok_queue_release_free_block(queue, curr_block);
        return true;
    } else {
        head_block->head_index++;
        _ok_queue_get_value(head_block->values, value_size, head_block->head_index, value);
        OK_UNLOCK(&queue->head_lock);
        return true;
    }
}

OK_LIB_API void _ok_queue_push(struct _ok_queue *queue, size_t value_size, void *value) {
    OK_LOCK(&queue->tail_lock);
    struct _ok_queue_block *tail_block = atomic_load(&queue->tail_block);
    if (tail_block == NULL) {
        struct _ok_queue_block *root_block = _ok_queue_get_free_block(queue, value_size);
        root_block->next = NULL;
        root_block->head_index = 0;
        atomic_store(&root_block->tail_index, 1);
        _ok_queue_set_value(root_block->values, value_size, 1, value);
        atomic_store(&queue->tail_block, root_block);
        atomic_store(&queue->head_block, root_block);
    } else if (atomic_load(&tail_block->tail_index) == queue->block_capacity - 1) {
        struct _ok_queue_block *free_block = _ok_queue_get_free_block(queue, value_size);
        free_block->next = NULL;
        free_block->head_index = 0;
        atomic_store(&free_block->tail_index, 0);
        _ok_queue_set_value(free_block->values, value_size, 0, value);
        tail_block->next = free_block;
        atomic_store(&queue->tail_block, free_block);
        atomic_store(&tail_block->tail_index, queue->block_capacity);
    } else {
        size_t new_tail_index = atomic_load(&tail_block->tail_index) + 1;
        _ok_queue_set_value(tail_block->values, value_size, new_tail_index, value);
        atomic_store(&tail_block->tail_index, new_tail_index);
    }
    OK_UNLOCK(&queue->tail_lock);
}

OK_LIB_API void _ok_queue_init(struct _ok_queue *queue, size_t value_size, size_t capacity) {
    if (capacity < 4) {
        queue->block_capacity = 4;
    } else {
        queue->block_capacity = capacity;
    }
    struct _ok_queue_block *root_block = _ok_queue_new_block(queue, value_size);
    root_block->head_index = 0;
    root_block->next = NULL;
    atomic_store(&root_block->tail_index, 0);

    atomic_store(&queue->head_block, root_block);
    atomic_store(&queue->head_lock, false);

    atomic_store(&queue->tail_block, root_block);
    atomic_store(&queue->tail_lock, false);

    atomic_store(&queue->free_block, _ok_queue_new_block(queue, value_size));
}

OK_LIB_API void _ok_queue_deinit(struct _ok_queue *queue, size_t value_size,
                                 void (*deallocator)(void *)) {
    OK_LOCK(&queue->tail_lock);
    if (deallocator) {
        // Dequeue every element and deallocate it
        void *value = malloc(value_size);
        while (_ok_queue_pop(queue, value_size, value)) {
            deallocator(value);
        }
        free(value);
        OK_LOCK(&queue->head_lock);
        struct _ok_queue_block *head_block = atomic_load(&queue->head_block);
        struct _ok_queue_block *tail_block = atomic_load(&queue->tail_block);
        _ok_queue_free_block(head_block);
        if (tail_block != head_block) {
            // Never happens?
            _ok_queue_free_block(tail_block);
        }
        atomic_store(&queue->head_block, NULL);
        atomic_store(&queue->tail_block, NULL);
    } else {
        OK_LOCK(&queue->head_lock);
        struct _ok_queue_block *head_block = atomic_load(&queue->head_block);
        while (head_block) {
            struct _ok_queue_block *next_block = head_block->next;
            _ok_queue_free_block(head_block);
            head_block = next_block;
        }
        atomic_store(&queue->head_block, NULL);
        atomic_store(&queue->tail_block, NULL);
    }

    struct _ok_queue_block *free_block = atomic_load(&queue->free_block);
    _ok_queue_free_block(free_block);
    atomic_store(&queue->free_block, NULL);
    OK_UNLOCK(&queue->head_lock);
    OK_UNLOCK(&queue->tail_lock);
}



//ADDED BY INTROSCOPIA

typedef struct ok_map_of(const char*, int   ) map_str_int;
typedef struct ok_map_of(const char*, char  ) map_str_char; 
typedef struct ok_map_of(const char*, void* ) map_str_vptr;
typedef struct ok_map_of(        int, char* ) map_int_str;
typedef struct ok_map_of(       char, int   ) map_char_int;
typedef struct ok_map_of(        int, int   ) map_int_int;

typedef struct ok_vec_of(int) int_vec;
typedef struct ok_vec_of(const char *) str_vec;

//ADDED BY INTROSCOPIA




#if defined(__GNUC__)
#  pragma GCC diagnostic pop
#elif defined (_MSC_VER)
#  pragma warning(pop)
#endif

#endif // OK_LIB_DEFINE

// @endcond

#undef OK_LIB_API
#undef OK_LIB_DEFINE
#undef OK_LIB_DECLARE

#endif
