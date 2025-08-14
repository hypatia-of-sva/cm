#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

const char* c_headers =
"/* include all libc headers */\n"
"#include <errno.h>\n"
"#include <stddef.h>\n"
"#include <assert.h>\n"
"#include <ctype.h>\n"
"#include <locale.h>\n"
"#include <math.h>\n"
"#include <setjmp.h>\n"
"#include <signal.h>\n"
"#include <stdarg.h>\n"
"#include <stdio.h>\n"
"#include <stdlib.h>\n"
"#include <string.h>\n"
"#include <time.h>\n"
"#include <float.h>\n"
"#include <limits.h>\n"
"#if (defined(__STDC_VERSION__) && (__STDC_VERSION__ >= 199901L))\n"
"#include <fenv.h>\n"
"#include <inttypes.h>\n"
"#include <iso646.h>\n"
"#include <stdbool.h>\n"
"#include <stdint.h>\n"
"#include <tgmath.h>\n"
"#include <wchar.h>\n"
"#include <wctype.h>\n"
"#if !defined(__STDC_NO_COMPLEX__)\n"
"#include <complex.h>\n"
"#endif\n"
"#if (__STDC_VERSION__ >= 201112L)\n"
"#include <stdalign.h>\n"
"#include <stdnoreturn.h>\n"
"#include <uchar.h>\n"
"#if !defined(__STDC_NO_ATOMICS__)\n"
"#include <stdatomic.h>\n"
"#endif\n"
"#if !defined(__STDC_NO_THREADS__)\n"
"#include <threads.h>\n"
"#endif\n"
"#if (__STDC_VERSION__ >= 202311L)\n"
"#include <stdbit.h>\n"
"#include <stdckdint.h>\n"
"#endif\n"
"#endif\n"
"#endif\n";

const char* unix_headers =
"/* Include all non-extension posix headers on unix systems */\n"
"#if defined(__unix__)\n"
"#define _POSIX_C_SOURCE 202405L\n"
"#include <unistd.h>\n"
"#if (defined(_POSIX_VERSION) && (_POSIX_VERSION >= 200809L))\n"
"#include <aio.h>\n"
"#include <arpa/inet.h>\n"
"#include <cpio.h>\n"
"#include <dirent.h>\n"
"#include <dlfcn.h>\n"
"#include <fcntl.h>\n"
"#include <fnmatch.h>\n"
"#include <glob.h>\n"
"#include <grp.h>\n"
"#include <iconv.h>\n"
"#include <langinfo.h>\n"
"#include <monetary.h>\n"
"#include <net/if.h>\n"
"#include <netdb.h>\n"
"#include <netinet/in.h>\n"
"#include <netinet/tcp.h>\n"
"#include <nl_types.h>\n"
"#include <poll.h>\n"
"#include <pthread.h>\n"
"#include <pwd.h>\n"
"#include <regex.h>\n"
"#include <sched.h>\n"
"#include <semaphore.h>\n"
"#include <strings.h>\n"
"#include <sys/mman.h>\n"
"#include <sys/select.h>\n"
"#include <sys/socket.h>\n"
"#include <sys/stat.h>\n"
"#include <sys/statvfs.h>\n"
"#include <sys/times.h>\n"
"#include <sys/types.h>\n"
"#include <sys/un.h>\n"
"#include <sys/utsname.h>\n"
"#include <sys/wait.h>\n"
"#include <tar.h>\n"
"#include <termios.h>\n"
"#include <wordexp.h>\n"
"#if (_POSIX_VERSION >= 202405L)\n"
"#include <endian.h>\n"
"#include <libintl.h>\n"
"#endif\n"
"#endif\n"
"#endif\n";

void die(const char* str) {
    fprintf(stderr, str);
    exit(EXIT_FAILURE);
}
void usage(const char* program_name) {
    printf("Usage: %s [-o outputname] [-h headerfilename] filename\n"
           "Use the environment-variable CM_DEFAULT_HEADER_NAME to set a default header; otherwise, (input).h will be used.\n", program_name);
    exit(0);
}

/* file util functions */
bool file_exists(const char* path) {
    FILE *file = fopen(path,"rb");
    if (file != NULL) {
        fclose(file);
        return true;
    } else {
        return false;
    }
}
size_t filesize(const char* path) {
    struct stat buf;
    stat(path, &buf);
    off_t size = buf.st_size;
    return size;
}
void read_whole_file(const char* path, char** buf_out, size_t* len_out) {
    FILE *file;
    size_t fread_size;

    assert(buf_out != NULL && len_out != NULL);
    if(!file_exists(path)) die("Input file does not exist!\n");
    len_out[0] = filesize(path);
    buf_out[0] = calloc(len_out[0],1);
    file = fopen(path,"rb");
    if(file == NULL) die("Error when trying to open input file!\n");
    fread_size = fread(buf_out[0], 1, len_out[0], file);
    if(fread_size != len_out[0]) die("Error when trying to read input file data!\n");
    fclose(file);
}

bool code_section_begin(const char* str) {
    return (str[0] == '<' && str[1] == '?' && str[2] == 'c' && (str[3] == ' ' || str[3] == '\n'));
}
bool code_section_end(const char* str) {
    return (str[0] == '?' && str[1] == '>');
}

/* valid printable string chars are all the characters in the basic C90 character set that don't have to be escaped, i.e. except ", ', ?, \, and control characters */
bool valid_string_char(char c) {
    if(c >= 'a' && c <= 'z') return true;
    if(c >= 'A' && c <= 'Z') return true;
    if(c >= '0' && c <= '9') return true;
    if(c == '!' || c == '#' || c == '%' || c == '&' || c == '(' || c == ')'
                || c == '*' || c == '+' || c == ',' || c == '-' || c == '.' || c == '/'
                || c == ':' || c == ';' || c == '<' || c == '=' || c == '>' || c == '['
                || c == ']' || c == '^' || c == '_' || c == '{' || c == '|' || c == '}'
                || c == '~' || c == ' ')
        return true;
    return false;
}
void print_escape(FILE* fd, char c) {
    switch(c) {
        case '\"':
            fprintf(fd, "\\\"");
            break;
        case '\'':
            fprintf(fd, "\\\'");
            break;
        case '\?':
            fprintf(fd, "\\\?");
            break;
        case '\\':
            fprintf(fd, "\\\\");
            break;
        case '\a':
            fprintf(fd, "\\a");
            break;
        case '\b':
            fprintf(fd, "\\b");
            break;
        case '\f':
            fprintf(fd, "\\f");
            break;
        case '\n':
            fprintf(fd, "\\n");
            break;
        case '\r':
            fprintf(fd, "\\r");
            break;
        case '\t':
            fprintf(fd, "\\t");
            break;
        case '\v':
            fprintf(fd, "\\v");
            break;
        default:
            fprintf(fd, "\\x%x",c);
            break;
    }
}

void parse_and_write_output(const char* input_buf, size_t input_len, const char* header_file_name, const char* output_path) {
    FILE *file;
    bool in_code_section = false;
    bool in_printf_section = false;
    int cur = 0;

    assert(input_buf != NULL && input_len > 3 && header_file_name != NULL && output_path != NULL);
    if (file_exists(output_path)) die("Output file does already exist!\n");
    file = fopen(output_path,"wb");
    if(file == NULL) die("Error when trying to create output file!\n");

    fprintf(file, "#include \"%s\"\nint main(int argc, char** argv) {\n", header_file_name);

    if(code_section_begin(input_buf)) {
        in_code_section = true;
        cur = 4;
    } else if (input_buf[0] == '@') {
        in_printf_section = true;
        cur = 1;
    }

    if(!in_code_section) fprintf(file, "printf(\"");

    while(cur < input_len) {
        if (!in_code_section && input_buf[cur] == '@') {
            if(!in_printf_section) {
                fprintf(file, "\");\n");
                fprintf(file, "printf(");
                in_printf_section = true;
            } else {
                fprintf(file, ");\n");
                fprintf(file, "printf(\"");
                in_printf_section = false;
            }
            cur++;
        } else if(!in_code_section && (input_len - cur) > 4 && code_section_begin(&input_buf[cur])) {
            if(!in_printf_section) {
                fprintf(file, "\");\n");
            }
            cur+=4;
            in_code_section = true;
        } else if(in_code_section && (input_len - cur) > 2 && code_section_end(&input_buf[cur])) {
            cur+=2;
            if(input_buf[cur] == '\n') {
                /* avoids ugly newlines in output */
                cur++;
            }
            if(!in_printf_section) {
                fprintf(file, "\nprintf(\"");
            }
            in_code_section = false;
        } else if(!in_code_section && !in_printf_section && input_buf[cur] == '\n') {
            fprintf(file, "\\n\"\n\"");
            cur++;
        } else if(!in_code_section && !in_printf_section && !valid_string_char(input_buf[cur])) {
            print_escape(file, input_buf[cur]);
            cur++;
        } else { /* by exhaustion also for !in_code_section && in_printf_section && input_buf[cur] != '@' */
            fputc(input_buf[cur], file);
            cur++;
        }
    }

    if(!in_code_section) fprintf(file, "\");\n");
    fprintf(file, "return 0;\n}\n");

    fclose(file);
}

void create_header(const char* header_file_name) {
    FILE *file;

    if(file_exists(header_file_name)) return;

    file = fopen(header_file_name,"wb");
    if(file == NULL) die("Error when trying to create header file!\n");

    fprintf(file, unix_headers);
    fprintf(file, "\n\n");
    fprintf(file, c_headers);

    fclose(file);
}

int main(int argc, char** argv) {
    char* input = NULL; char* output = NULL; char* header = NULL;
    char* input_buf; size_t input_len;
    bool header_alloc = false, output_alloc = false;
    int curr_arg = 1;

    if(argc < 2) usage(argv[0]);
    if(strcmp(argv[curr_arg],"-o") == 0) {
        output = argv[curr_arg+1];
        argc -= 2; curr_arg += 2;
    }
    if(argc < 2) usage(argv[0]);
    if(strcmp(argv[curr_arg],"-h") == 0) {
        header = argv[curr_arg+1];
        argc -= 2; curr_arg += 2;
    }
    if(argc < 2) usage(argv[0]);
    input = argv[curr_arg];

    if(header == NULL) {
        header = getenv("CM_DEFAULT_HEADER_NAME");
        if(header == NULL) {
            header = malloc(strlen(input)+10);
            snprintf(header, strlen(input)+10, "%s.h", input);
            header_alloc = true;
        }
    }
    if(output == NULL) {
        output = malloc(strlen(input)+10);
        snprintf(output, strlen(input)+10, "%s.out.c", input);
        output_alloc = true;
    }

    printf("Input from file: %s\nOutput into file: %s\nUsed header: %s\n", input, output, header);

    create_header(header);

    read_whole_file(input, &input_buf, &input_len);

    parse_and_write_output(input_buf, input_len, header, output);

    free(input_buf);

    if(header_alloc) free(header);
    if(output_alloc) free(output);

    return 0;
}
