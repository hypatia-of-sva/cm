#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <sys/stat.h>

void die(const char* str) {
    fprintf(stderr, str);
    exit(EXIT_FAILURE);
}
void usage(const char* program_name) {
    printf("Usage: %s [-o outputname] [-h headerfilename] filename\n"
           "Use the environment-variable CMC_DEFAULT_HEADER_NAME to set a default header; otherwise, (input).h will be used.\n", program_name);
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
            fprintf(fd, "\\\a");
            break;
        case '\b':
            fprintf(fd, "\\\b");
            break;
        case '\f':
            fprintf(fd, "\\\f");
            break;
        case '\n':
            fprintf(fd, "\\\n");
            break;
        case '\r':
            fprintf(fd, "\\\r");
            break;
        case '\t':
            fprintf(fd, "\\\t");
            break;
        case '\v':
            fprintf(fd, "\\\v");
            break;
        default:
            fprintf(fd, "\\x%x",c);
            break;
    }
}

void parse_and_write_output(const char* input_buf, size_t input_len, const char* header_file_name, const char* output_path) {
    FILE *file;
    bool in_code_section = false;
    int cur = 0;

    assert(input_buf != NULL && input_len > 3 && header_file_name != NULL && output_path != NULL);
    if (file_exists(output_path)) die("Output file does already exist!\n");
    file = fopen(output_path,"wb");
    if(file == NULL) die("Error when trying to create output file!\n");

    fprintf(file, "#include \"%s\"\nint main(int argc, char** argv) {\n", header_file_name);

    if(code_section_begin(input_buf)) {
        in_code_section = true;
        cur = 4;
    }

    if(!in_code_section) fprintf(file, "puts(\"");

    while(cur < input_len) {
        if(!in_code_section && (input_len - cur) > 4 && code_section_begin(&input_buf[cur])) {
            fprintf(file, "\");\n");
            cur+=4;
            in_code_section = true;
        } else if(in_code_section && (input_len - cur) > 2 && code_section_end(&input_buf[cur])) {
            fprintf(file, "\nputs(\"");
            cur+=2;
            in_code_section = false;
        } else if(!in_code_section && input_buf[cur] == '\n') {
            fprintf(file, "\"\n\"");
            cur++;
        } else if(!in_code_section && !valid_string_char(input_buf[cur])) {
            print_escape(file, input_buf[cur]);
            cur++;
        } else {
            fputc(input_buf[cur], file);
            cur++;
        }
    }

    if(!in_code_section) fprintf(file, "\");\n");
    fprintf(file, "return 0;\n}\n");

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
        header = getenv("CMC_DEFAULT_HEADER_NAME");
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

    read_whole_file(input, &input_buf, &input_len);

    parse_and_write_output(input_buf, input_len, header, output);

    free(input_buf);

    if(header_alloc) free(header);
    if(output_alloc) free(output);

    return 0;
}
