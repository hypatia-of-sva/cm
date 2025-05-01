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
puts(".... other text ..."
"");
  /* Here we have C code that can algorithmically generate output by printing to stdout.
   * libc and unix facilities, if available, can be used by default. */
  for(int i = 0; i < 100; i++) {
    printf("cos(%i) = %f", i, cos(i*1.0));
  }

puts(""
"... more other text .."
"");
return 0;
}

```

with a generated header `example.h` to include libc and posix headers, if no special header already exists with the name. The name of this header can also be overriden by a command-line variable or an environment variable. The real output of the program can now be reached by compiling this file with any c-compiler and running it in turn.

## Full usage syntax

Usage: `cm \[-o outputname\] \[-h headerfilename\] filename`
Use the environment-variable CM_DEFAULT_HEADER_NAME to set a default header; otherwise, (input).h will be used.

## dependencies

cm itself depends only on libc and on stat to determine file size, it thus compiles for example in gcc or mingw with no added library options.
Code generated with cm, especially when using custom headers, obviously can make use of any kinds of external dependencies.

## example: using cm to generate native functions for different types

One basic example that can be cumbersome in C, is generating basically the same function for different types. Here is an example of how to create different functions for the same quicksort implementations for different types:

```c
<?c
char* types[10] = {"int8_t", "uint8_t", "int16_t", "uint16_t", "int32_t", "uint32_t", "int64_t", "uint64_t", "float", "double"};
char* functname_suffix[10] = {"i8", "u8", "i16", "u16", "i32", "u32", "i64", "u64", "f32", "f64"};
for(int i = 0; i < 10; i++) {
?>
#define T ?c puts(types[i]); ?>
void quick_sort_<?c puts(functname_suffix[i]); ?>(T* array, size_t len) {
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

This will generate the 10 functions to sort arrays of native integer and floating point types.
