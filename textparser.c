#include <textparser.h>
#include <adv_regex.h>
#include <re.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <sys/mman.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

//#include <utf8proc.h>


#define MAX_PARSE_SIZE (16 * 1024 * 1024) /* 16MB */

enum textparser_bom {
    NO_BOM,
    BOM_UTF_8,
    BOM_UTF_16_BE,
    BOM_UTF_16_LE,
    BOM_UTF_32_BE,
    BOM_UTF_32_LE,
    BOM_UTF_7_1,
    BOM_UTF_7_2,
    BOM_UTF_7_3,
    BOM_UTF_7_4,
    BOM_UTF_7_5,
    BOM_UTF_1,
    BOM_UTF_EBCDIC,
    BOM_UTF_SCSU,
    BOM_UTF_BOCU1,
    BOM_UTF_GB_18030,
};

struct textparser_handle {
    int mmap_fd;
    void *mmap_addr;
    size_t mmap_size;
    enum textparser_bom bom;
    enum textparse_text_format text_format;
    char *text_addr;
    size_t text_size;
    ulong no_lines;
    ulong lines[];
} __attribute__((aligned(1)));

int textparse_openfile(const char *pathname, int text_format, void **handle)
{
    struct textparser_handle local_hnd;
    struct stat fd_stat;
    int err = 0;

    //uint8_t bytes[] = {0xd0, 0x91, 0xd0, 0xbe, 0xd1, 0x80, 0xd0, 0xbe};
    //uint32_t ch = 0;
    //int ret = utf8proc_iterate(bytes, 6, &ch);

    memset(&local_hnd, 0, sizeof(local_hnd));

    local_hnd.mmap_fd = open(pathname, O_RDONLY);
    if (local_hnd.mmap_fd <= 0) {
        err = 1;
        goto err;
    }

    if (fstat(local_hnd.mmap_fd, &fd_stat)) {
        err = 2;
        goto err;
    }

    if (fd_stat.st_size > MAX_PARSE_SIZE) {
        err = 3;
        goto err;
    }

    local_hnd.mmap_size = fd_stat.st_size;

    local_hnd.mmap_addr = mmap(NULL, local_hnd.mmap_size, PROT_READ, MAP_PRIVATE, local_hnd.mmap_fd, 0);
    if (local_hnd.mmap_addr == NULL) {
        err = 4;
        goto err;
    }

    local_hnd.text_addr = local_hnd.mmap_addr;
    local_hnd.text_size = local_hnd.mmap_size;

    if ((local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xfe')&&(local_hnd.text_addr[1] == '\xbb')&&(local_hnd.text_addr[2] == '\xbf')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = BOM_UTF_8;
    } else if ((local_hnd.text_size >= 2)&&(local_hnd.text_addr[0] == '\xfe')&&(local_hnd.text_addr[1] == '\xff')) {
        local_hnd.text_addr += 2;
        local_hnd.text_size -= 2;
        local_hnd.bom = BOM_UTF_16_BE;
    } else if ((local_hnd.text_size >= 2)&&(local_hnd.text_addr[0] == '\xff')&&(local_hnd.text_addr[1] == '\xfe')) {
        local_hnd.text_addr += 2;
        local_hnd.text_size -= 2;
        local_hnd.bom = BOM_UTF_16_LE;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x00')&&(local_hnd.text_addr[1] == '\x00')&&(local_hnd.text_addr[2] == '\xfe')&&(local_hnd.text_addr[3] == '\xff')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_32_BE;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\xff')&&(local_hnd.text_addr[1] == '\xfe')&&(local_hnd.text_addr[2] == '\x00')&&(local_hnd.text_addr[3] == '\x00')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_32_LE;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x38')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_7_1;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x39')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_7_2;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x2b')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_7_3;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x2f')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_7_4;
    } else if ((local_hnd.text_size >= 5)&&(local_hnd.text_addr[0] == '\x2b')&&(local_hnd.text_addr[1] == '\x2f')&&(local_hnd.text_addr[2] == '\x76')&&(local_hnd.text_addr[3] == '\x38')&&(local_hnd.text_addr[4] == '\x2d')) {
        local_hnd.text_addr += 5;
        local_hnd.text_size -= 5;
        local_hnd.bom = BOM_UTF_7_5;
    } else if ((local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xf7')&&(local_hnd.text_addr[1] == '\x64')&&(local_hnd.text_addr[2] == '\x4c')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = BOM_UTF_1;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\xdd')&&(local_hnd.text_addr[1] == '\x73')&&(local_hnd.text_addr[2] == '\x66')&&(local_hnd.text_addr[3] == '\x73')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_EBCDIC;
    } else if ((local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\x0e')&&(local_hnd.text_addr[1] == '\xfe')&&(local_hnd.text_addr[2] == '\xff')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = BOM_UTF_SCSU;
    } else if ((local_hnd.text_size >= 3)&&(local_hnd.text_addr[0] == '\xfb')&&(local_hnd.text_addr[1] == '\xee')&&(local_hnd.text_addr[2] == '\x28')) {
        local_hnd.text_addr += 3;
        local_hnd.text_size -= 3;
        local_hnd.bom = BOM_UTF_BOCU1;
    } else if ((local_hnd.text_size >= 4)&&(local_hnd.text_addr[0] == '\x84')&&(local_hnd.text_addr[1] == '\x31')&&(local_hnd.text_addr[2] == '\x95')&&(local_hnd.text_addr[3] == '\x33')) {
        local_hnd.text_addr += 4;
        local_hnd.text_size -= 4;
        local_hnd.bom = BOM_UTF_GB_18030;
    } else {
        local_hnd.bom = NO_BOM;
    }

    // TODO: Currently don't support BOM
    if (local_hnd.bom != NO_BOM) {
        err = 5;
        goto err;
    }

    local_hnd.no_lines = 0;
    int cur_line_pos = 0;

    local_hnd.text_format = text_format;

    switch(text_format) {
    case TEXTPARSE_LATIN_1:
        for(int ch = 0; ch < local_hnd.text_size; ch++) {
            if (local_hnd.text_addr[ch] == '\n')
                local_hnd.no_lines++;
        }

        *handle = malloc(sizeof(struct textparser_handle) + (sizeof(ulong) * (local_hnd.no_lines)));
        if (*handle == NULL) {
            err = 6;
            goto err;
        }

        memcpy(*handle, &local_hnd, sizeof(struct textparser_handle));

        for(int ch = 0; ch < local_hnd.text_size; ch++) {
            if (local_hnd.text_addr[ch] == '\n') {
                ((struct textparser_handle*)*handle)->lines[cur_line_pos++] = ch;
            }
        }
        break;
    case TEXTPARSE_UTF_8:
        break;
    case TEXTPARSE_UNICODE:
        for(int ch = 0; ch < local_hnd.text_size / sizeof(u_int16_t); ch++) {
            if (((u_int16_t *)local_hnd.text_addr)[ch] == '\n')
                local_hnd.no_lines++;
        }

        *handle = malloc(sizeof(struct textparser_handle) + (sizeof(ulong) * (local_hnd.no_lines)));
        if (*handle == NULL) {
            err = 6;
            goto err;
        }

        memcpy(*handle, &local_hnd, sizeof(struct textparser_handle));

        for(int ch = 0; ch < local_hnd.text_size / sizeof(u_int16_t); ch++) {
            if (((u_int16_t *)local_hnd.text_addr)[ch] == '\n') {
                ((struct textparser_handle*)*handle)->lines[cur_line_pos++] = ch;
            }
        }
        break;
    default:
        err = 7;
        goto err;
        break;
    }

    return 0;

err:
    if (local_hnd.mmap_addr)
        munmap(local_hnd.mmap_addr, local_hnd.mmap_size);

    if (local_hnd.mmap_fd)
        close(local_hnd.mmap_fd);

    return err;
}

void textparse_close(void *handle)
{
    int mmap_fd = 0;
    void *mmap_addr = NULL;
    size_t mmap_size = 0;

    if (handle == NULL)
        return;

    mmap_fd = ((struct textparser_handle *)handle)->mmap_fd;
    mmap_addr = ((struct textparser_handle *)handle)->mmap_addr;
    mmap_size = ((struct textparser_handle *)handle)->mmap_size;

    munmap(mmap_addr, mmap_size);
    close(mmap_fd);

    free(handle);
}

static void textparse_skipwhitespace(const struct textparser_handle *int_handle, int *index)
{
    if (int_handle->text_format == TEXTPARSE_UNICODE)
    {
        while(1)
        {
            u_int16_t ch = *(u_int16_t *)(int_handle->text_addr + (*index));

            if ((ch != ' ')&&(ch != '\t')&&(ch != '\r')&&(ch != '\n'))
                break;

            *index = *index + 2;
        }
    }
    else
    {
        while(1)
        {
            char ch = int_handle->text_addr[*index];

            if ((ch != ' ')&&(ch != '\t')&&(ch != '\r')&&(ch != '\n'))
                break;

            *index = *index + 1;
        }
    }
}

static int textparse_find_token(const struct textparser_handle *int_handle, const language_definition_t *definition, int token, int offset)
{
    void *found  = NULL;
    found = adv_regex_find_pattern(definition->tokens[token].start_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr, int_handle->text_addr + int_handle->text_size, NULL);
    if (!found)
        return -1;

    return (char *)found - int_handle->text_addr;
}

static textparse_token_item_t *textparse_parse_token(const struct textparser_handle *int_handle, const language_definition_t *definition, int token, int offset)
{
    textparse_token_item_t *ret = NULL;

    int len = 0;
    int token_start = 0;
    int current_offset = offset;

    ret = malloc(sizeof(textparse_token_item_t));
    memset(ret, 0, sizeof(textparse_token_item_t));

    const textparse_token_t *token_def = &definition->tokens[token];

    ret->token_id = token;

    token_start = adv_regex_find_pattern(token_def->start_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr, current_offset, &len);
    if (token_start < 0)
    {
        ret->error = "Can't find start of the token!";
        ret->position = current_offset;
        return ret;
    }

    ret->position = current_offset + token_start;
    current_offset += token_start + len;

    textparse_skipwhitespace(int_handle, &current_offset);

    if (token_def->nested_tokens)
    {
        // TODO: Resume from here!

        token_start = adv_regex_find_pattern(token_def->end_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr, current_offset, &len);
    }

    ret->len = current_offset - offset;

    return ret;
}

int textparse_parse(void *handle, const language_definition_t *definition)
{
    struct textparser_handle *int_handle = (struct textparser_handle *)handle;

    const char *text = int_handle->text_addr;
    size_t size = int_handle->text_size;
    size_t pos = 0;

    while(1) {
        int closesed_token = -1;
        int closesed_offset = size - pos;

        for (int c = 0; definition->starts_with[c] != -1; c++) {
            int token_id = definition->starts_with[c];
            int offset = textparse_find_token(handle, definition, token_id, pos);
return 0;
            if ((offset >= 0)&&(offset < closesed_offset)) {
                closesed_token = token_id;
                closesed_offset = offset;
            }

            if (closesed_offset == 0)
                break;
        }

        if (closesed_offset == -1)
            break;

        textparse_token_item_t *token_item = textparse_parse_token(handle, definition, closesed_token, closesed_offset);
    }

    return 0;
}
