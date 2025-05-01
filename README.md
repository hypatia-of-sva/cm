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

with a generated header `example.h` to include libc and posix headers. The name of this header can be overriden by a command-line variable or an environment variable.

Full usage:

Usage: `cm \[-o outputname\] \[-h headerfilename\] filename`
Use the environment-variable CMC_DEFAULT_HEADER_NAME to set a default header; otherwise, (input).h will be used.

