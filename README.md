# cm - C Macro processor

This little tool makes use of C as a macro processor to create any other text (including other C programs!). The syntax is like this:

```
.... other text ...
<?c
  /* Here we have C code that can algorithmically generate output by printing to stdout.
   * libc and unix facilities, if available, can be used by default. */
  for(int i = 0; i < 100; i++) {
    printf("cos(%i) = %f", i, cos(i*1.0));
  }
?>
... more other text ..
```

This will be translated into
```c
#include "example.h"
int main(int argc, char** argv) {
printf(".... other text ..."
"");
  /* Here we have C code that can algorithmically generate output by printing to stdout.
   * libc and unix facilities, if available, can be used by default. */
  for(int i = 0; i < 100; i++) {
    printf("cos(%i) = %f", i, cos(i*1.0));
  }

printf(""
"... more other text .."
"");
return 0;
}

```

with a generated header `example.h` to include libc and posix headers, if no special header already exists with the name. The name of this header can also be overriden by a command-line variable or an environment variable. The real output of the program can now be reached by compiling this file with any c-compiler and running it in turn.

## Why cm and not any other macro processor (m4, python scripts, zig, gpp etc.)?

Because it also operates in C. This means you can use all of the code you have written or otherwise generated within it, effectively making C into a language with compile-time execution without adding anything to it, just by adding another compiler-and-run pass to your build system.

## Full usage syntax

Usage: `cm [-o outputname] [-h headerfilename] filename`
Use the environment-variable CM_DEFAULT_HEADER_NAME to set a default header; otherwise, (input).h will be used.

## dependencies

cm itself depends only on libc and on stat to determine file size, it thus compiles for example in gcc or mingw with no added library options.
Code generated with cm, especially when using custom headers, obviously can make use of any kinds of external dependencies.

## example: using cm to generate native functions for different types

One basic example that can be cumbersome in C, is generating basically the same function for different types. Here is an example of how to create different functions for the same quicksort implementations for different types:

```c
<?c
char* types[10] = {"int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t",
  "int64_t", "uint64_t", "float", "double"};
char* functname_suffix[10] = {"i8", "u8", "i16", "u16", "i32", "u32",
  "i64", "u64", "f32", "f64"};
for(int i = 0; i < 10; i++) {
?>
#define T <?c printf(types[i]); ?>
void quick_sort_<?c printf(functname_suffix[i]); ?>(T* array, size_t len) {
    /* iterative quicksort, adapted from https://www.geeksforgeeks.org/iterative-quick-sort/ */
    int* stack = calloc(len,sizeof(int));
    stack[0] = 0;
    stack[1] = len-1;
    int top = 1;
    while (top >= 0) {
        int h = stack[top--];
        int l = stack[top--];

        /* partition: */
        T x = array[h]; /* pivot */
        int i = l-1;
        for(int j = l; j < h; j++) {
            if(array[j] <= x) {
                i++;
                T tmp = array[i];
                array[i] = array[j];
                array[j] = tmp;
            }
        }
        T tmp = array[i+1];
        array[i+1] = array[h];
        array[h] = tmp;

        if (i > l) {
            stack[++top] = l;
            stack[++top] = i;
        }
        if (i+2 < h) {
            stack[++top] = i+2;
            stack[++top] = h;
        }
    }
    free(stack);
}
#undef T
<?c
}
?>
```

This will generate the 10 functions to sort arrays of native integer and floating point types; this is the first one:

```c
#define T int8_t
void quick_sort_i8(T* array, size_t len) {
    /* iterative quicksort, adapted from https://www.geeksforgeeks.org/iterative-quick-sort/ */
    int* stack = calloc(len,sizeof(int));
    stack[0] = 0;
    stack[1] = len-1;
    int top = 1;
    while (top >= 0) {
        int h = stack[top--];
        int l = stack[top--];

        /* partition: */
        T x = array[h]; /* pivot */
        int i = l-1;
        for(int j = l; j < h; j++) {
            if(array[j] <= x) {
                i++;
                T tmp = array[i];
                array[i] = array[j];
                array[j] = tmp;
            }
        }
        T tmp = array[i+1];
        array[i+1] = array[h];
        array[h] = tmp;

        if (i > l) {
            stack[++top] = l;
            stack[++top] = i;
        }
        if (i+2 < h) {
            stack[++top] = i+2;
            stack[++top] = h;
        }
    }
    free(stack);
}
#undef T
```

## New - Direct variable mode!

Since it was kind of clumsy to enter strings multiple times, where it wasn't easily supported by `#define`s, we added a new direct variable output mode. This means, that for example the following code:


```c
/* generated list of checked additions: */

<?c
#define INT_TYPE_NR 10
const char* int_types[INT_TYPE_NR] = {"int8", "int16", "int32", "int64", "int128",
                            "uint8", "uint16", "uint32", "uint64", "uint128"};

for(int i = 0; i < INT_TYPE_NR; i++) {
char* t1 = int_types[i];
for(int j = 0; j < INT_TYPE_NR; j++) {
char* t2 = int_types[j];
for(int k = 0; k < INT_TYPE_NR; k++) {
char* t3 = int_types[k];
?>
bool32_t @t1@_checked_addition_of_@t2@_and_@t3@(@t1@_t* result, @t2@_t a, @t3@_t b);
<?c
}
}
}
?>
```

Will generate the following list (only the beginning, since it is over 1000 lines long):
```c
/* generated list of checked additions: */

bool32_t int8_checked_addition_of_int8_and_int8(int8_t* result, int8_t a, int8_t b);
bool32_t int8_checked_addition_of_int8_and_int16(int8_t* result, int8_t a, int16_t b);
bool32_t int8_checked_addition_of_int8_and_int32(int8_t* result, int8_t a, int32_t b);
bool32_t int8_checked_addition_of_int8_and_int64(int8_t* result, int8_t a, int64_t b);
bool32_t int8_checked_addition_of_int8_and_int128(int8_t* result, int8_t a, int128_t b);
bool32_t int8_checked_addition_of_int8_and_uint8(int8_t* result, int8_t a, uint8_t b);
bool32_t int8_checked_addition_of_int8_and_uint16(int8_t* result, int8_t a, uint16_t b);
bool32_t int8_checked_addition_of_int8_and_uint32(int8_t* result, int8_t a, uint32_t b);
bool32_t int8_checked_addition_of_int8_and_uint64(int8_t* result, int8_t a, uint64_t b);
bool32_t int8_checked_addition_of_int8_and_uint128(int8_t* result, int8_t a, uint128_t b);
bool32_t int8_checked_addition_of_int16_and_int8(int8_t* result, int16_t a, int8_t b);
bool32_t int8_checked_addition_of_int16_and_int16(int8_t* result, int16_t a, int16_t b);
bool32_t int8_checked_addition_of_int16_and_int32(int8_t* result, int16_t a, int32_t b);
bool32_t int8_checked_addition_of_int16_and_int64(int8_t* result, int16_t a, int64_t b);
bool32_t int8_checked_addition_of_int16_and_int128(int8_t* result, int16_t a, int128_t b);
bool32_t int8_checked_addition_of_int16_and_uint8(int8_t* result, int16_t a, uint8_t b);
bool32_t int8_checked_addition_of_int16_and_uint16(int8_t* result, int16_t a, uint16_t b);
bool32_t int8_checked_addition_of_int16_and_uint32(int8_t* result, int16_t a, uint32_t b);
bool32_t int8_checked_addition_of_int16_and_uint64(int8_t* result, int16_t a, uint64_t b);
bool32_t int8_checked_addition_of_int16_and_uint128(int8_t* result, int16_t a, uint128_t b);
...
```

The general syntax for this direct input is `@`something`@`; this will generate `printf(`something`);` in the output, the something can be a variable or a more complicated expression with a format string and multiple other parameters.
