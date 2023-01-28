#include <stdio.h>
#include <textparser.h>
#include <adv_regex.h>

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
    const language_definition_t *language;
    void *mmap_addr;
    size_t mmap_size;
    enum textparser_bom bom;
    enum textparse_text_format text_format;
    textparse_token_item_t *first_item;
    bool fatal_error;
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

    int fd = open(pathname, O_RDONLY);
    if (fd <= 0) {
        err = 1;
        goto err;
    }

    if (fstat(fd, &fd_stat)) {
        err = 2;
        goto err;
    }

    if (fd_stat.st_size > MAX_PARSE_SIZE) {
        err = 3;
        goto err;
    }

    local_hnd.mmap_size = fd_stat.st_size;

    local_hnd.mmap_addr = mmap(NULL, local_hnd.mmap_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (local_hnd.mmap_addr == NULL) {
        err = 4;
        goto err;
    }

    close(fd);

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

    return err;
}

void free_item_tree(textparse_token_item_t *item)
{
    textparse_token_item_t *next = NULL;
    textparse_token_item_t *tmp = NULL;

    if (item == NULL)
        return;

    if (item->child)
    {
        free_item_tree(item->child);
        item->child = NULL;
    }

    next = item->next;
    if (next)
    {
        free_item_tree(next);

        item->next = NULL;
    }

    printf("free_item_tree - free\n");
    free(item);
}

void textparse_close(void *handle)
{
    struct textparser_handle *int_handle = (struct textparser_handle *)handle;
    textparse_token_item_t *item = NULL;
    void *mmap_addr = NULL;
    size_t mmap_size = 0;

    if (int_handle == NULL)
        return;

    mmap_addr = int_handle->mmap_addr;
    mmap_size = int_handle->mmap_size;

    munmap(mmap_addr, mmap_size);

    item = int_handle->first_item;
    if (item)
    {
        free_item_tree(item);
        item = NULL;
    }

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
    return adv_regex_find_pattern(definition->tokens[token].start_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr + offset, int_handle->text_size - offset, NULL);
}

static textparse_token_item_t *textparse_parse_token(struct textparser_handle *int_handle, const language_definition_t *definition, int token_id, int offset)
{
    textparse_token_item_t *ret = NULL;

    int len = 0;
    int token_start = 0;
    int token_end = 0;
    int current_offset = offset;

    ret = malloc(sizeof(textparse_token_item_t));
    memset(ret, 0, sizeof(textparse_token_item_t));

    const textparse_token_t *token_def = &definition->tokens[token_id];

    ret->token_id = token_id;

    token_start = adv_regex_find_pattern(token_def->start_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr + offset, int_handle->text_size - offset, &len);
    if (token_start < 0)
    {
        ret->error = "Can't find start of the token!";
        ret->position = offset;
        int_handle->fatal_error = true;
        return ret;
    }

    ret->position = offset + token_start;
    current_offset = ret->position + len;

    /*if (token_def->nested_tokens)
    {
        textparse_skipwhitespace(int_handle, &current_offset);
        // TODO: Resume from here!

        token_start = adv_regex_find_pattern(token_def->end_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr, current_offset, &len);
    }*/

    if (!token_def->only_start_tag)
    {
        token_end = adv_regex_find_pattern(token_def->end_string, ADV_REGEX_TEXT_LATIN1, int_handle->text_addr + current_offset, int_handle->text_size - current_offset, &len);
        if (token_end >= 0)
        {
            current_offset += len;
            ret->len = current_offset - ret->position;
        }
    }

    return ret;
}

int textparse_parse(void *handle, const language_definition_t *definition)
{
    struct textparser_handle *int_handle = (struct textparser_handle *)handle;

    textparse_token_item_t *prev_item = NULL;

    const char *text = int_handle->text_addr;
    size_t size = int_handle->text_size;
    size_t pos = 0;

    int_handle->language = definition;

    while(1) {
        int closesed_token = -1;
        int closesed_offset = size - pos;

        for (int c = 0; definition->starts_with[c] != -1; c++) {
            int token_id = definition->starts_with[c];
            int offset = textparse_find_token(handle, definition, token_id, pos);
            if ((offset >= 0)&&(offset < closesed_offset)) {
                closesed_token = token_id;
                closesed_offset = pos + offset;
            }

            if (closesed_offset == 0)
                break;
        }

        if ((closesed_offset < 0)||(closesed_token < 0))
            break;

        textparse_token_item_t *token_item = textparse_parse_token(handle, definition, closesed_token, closesed_offset);

        if (int_handle->first_item == NULL)
            int_handle->first_item = token_item;

        if (prev_item)
            prev_item->next = token_item;

        if ((int_handle->fatal_error)||(token_item->token_id == -1)||(token_item->len <= 0))
            return -1;

        pos = token_item->position + token_item->len;

        prev_item = token_item;
    }

    return 0;
}

textparse_token_item_t *textparse_get_first_token(void *handle)
{
    struct textparser_handle *int_handle = (struct textparser_handle *)handle;

    if (int_handle == NULL)
        return NULL;

    return int_handle->first_item;
}

const char *textparse_get_token_id_name(void *handle, int token_id)
{
    struct textparser_handle *int_handle = (struct textparser_handle *)handle;

    if ((int_handle == NULL)||(token_id < 0))
        return NULL;

    for (int c = 0; c <= token_id; c++)
    {
        if (int_handle->language->tokens[c].name == NULL)
            return NULL;

        if (c == token_id)
            return int_handle->language->tokens[c].name;
    }

    return NULL;
}

char *textparse_get_token_text(void *handle, textparse_token_item_t *item)
{    
    char *ret = NULL;

    struct textparser_handle *int_handle = (struct textparser_handle *)handle;

    if ((int_handle == NULL)||(item == NULL)||(item->len <= 0))
        return NULL;

    ret = malloc(item->len + 1);
    strncpy(ret, int_handle->text_addr + item->position, item->len);
    ret[item->len] = '\0';

    return ret;
}
