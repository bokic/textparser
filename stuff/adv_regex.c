#include "adv_regex.h"

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>


#define ARRAY_SIZE(arr) (sizeof((arr)) / sizeof((arr)[0]))

enum { ERROR = -1, ANY, BEGIN, END, ZERO_OR_ONE_OF_PREV, ZERO_OR_MORE_OF_PREV, ONE_OR_MORE_OF_PREV, STRING, CHAR, CHAR_CLASS, INV_CHAR_CLASS, DIGIT, NOT_DIGIT, ALPHA, NOT_ALPHA, WHITESPACE, NOT_WHITESPACE, BOOLEAN_OR, GROUP };

struct adv_regex_pattern_item {
    int type;
    int child;
    int next;
    union {
        unsigned char ch;
        void *data;
    };
} adv_regex_pattern_item_t;

struct adv_regex_pattern_int {

    int item_cnt;
    struct adv_regex_pattern_item items[0];

} adv_regex_pattern_int_t;

static bool adv_regex_add_item_to_pattern(struct adv_regex_pattern_int **ret, int type, char ch)
{
    int malloc_size = offsetof(struct adv_regex_pattern_int, items);

    if ((ret == NULL)||((*ret) == NULL))
        return ret;

    malloc_size += ((*ret)->item_cnt + 1) * sizeof(struct adv_regex_pattern_item);

    *ret = realloc(*ret, malloc_size);
    if (*ret == NULL)
        return false;

    (*ret)->items[(*ret)->item_cnt].type = CHAR;
    (*ret)->items[(*ret)->item_cnt].ch = ch;

    (*ret)->item_cnt++;

    return true;
}

static bool adv_regex_add_ch_to_pattern(struct adv_regex_pattern_int **ret, unsigned char ch)
{
    if ((ret == NULL)||((*ret) == NULL))
        return ret;

    if ((*ret)->item_cnt > 0)
    {
        int cnt = (*ret)->item_cnt - 1;
        int prev_type = (*ret)->items[cnt].type;

        if (prev_type == STRING)
        {
            int len = strlen((char *)(*ret)->items[cnt].data);
            (*ret)->items[cnt].data = realloc((*ret)->items[cnt].data, len + 2);
            ((unsigned char *)(*ret)->items[cnt].data)[len] = ch;
            ((unsigned char *)(*ret)->items[cnt].data)[len + 1] = '\0';

            return true;
        }
        else if (prev_type == CHAR)
        {
            unsigned char old_ch = (*ret)->items[cnt].ch;
            (*ret)->items[cnt].type = STRING;
            (*ret)->items[cnt].data = malloc(3);
            ((unsigned char *)(*ret)->items[cnt].data)[0] = old_ch;
            ((unsigned char *)(*ret)->items[cnt].data)[1] = ch;
            ((unsigned char *)(*ret)->items[cnt].data)[2] = '\0';

            return true;
        }
    }

    return adv_regex_add_item_to_pattern(ret, CHAR, ch);
}

static bool adv_regex_compile_recursive(struct adv_regex_pattern_int **ret, const char* pattern, int len, int *pos)
{
    int start_pos = 0;

    if ((ret == NULL)||((*ret) == NULL)||(pattern == NULL))
        return ret;

    start_pos = *pos;

    while(*pos < len)
    {
        unsigned char ch = pattern[*pos];

        switch(ch)
        {
        case '^': adv_regex_add_item_to_pattern(ret, BEGIN, 0);                break;
        case '$': adv_regex_add_item_to_pattern(ret, END, 0);                  break;
        case '.': adv_regex_add_item_to_pattern(ret, ANY, 0);                  break;
        case '*': adv_regex_add_item_to_pattern(ret, ZERO_OR_MORE_OF_PREV, 0); break;
        case '+': adv_regex_add_item_to_pattern(ret, ONE_OR_MORE_OF_PREV, 0);  break;
        case '?': adv_regex_add_item_to_pattern(ret, ZERO_OR_ONE_OF_PREV, 0);  break;
        case '|': adv_regex_add_item_to_pattern(ret, BOOLEAN_OR, 0);           break; // TODO: Fix here next!
        case '\\':
            (*pos)++;

            if (*pos == len)
                return false;

            ch = pattern[*pos];
            switch(ch)
            {
            case 'd': adv_regex_add_item_to_pattern(ret, DIGIT, 0);          break;
            case 'D': adv_regex_add_item_to_pattern(ret, NOT_DIGIT, 0);      break;
            case 'w': adv_regex_add_item_to_pattern(ret, ALPHA, 0);          break;
            case 'W': adv_regex_add_item_to_pattern(ret, NOT_ALPHA, 0);      break;
            case 's': adv_regex_add_item_to_pattern(ret, WHITESPACE, 0);     break;
            case 'S': adv_regex_add_item_to_pattern(ret, NOT_WHITESPACE, 0); break;
            default:  adv_regex_add_ch_to_pattern(ret, ch);                  break;
            }
            break;
        case '[': break; // TODO: Fix here
        case '(': break; // TODO: Fix here
        case '{': break; // TODO: Fix here
        case ']':
        case ')':
        case '}': return false;
        default:   adv_regex_add_ch_to_pattern(ret, ch); break;
        }

        (*pos)++;
    }

    return true;
}

adv_regex_pattern adv_regex_compile(const char* pattern)
{
    struct adv_regex_pattern_int *ret = NULL;
    int malloc_size = 0;
    int pos = 0;
    int len = 0;

    if (pattern == NULL)
        return NULL;

    malloc_size = offsetof(struct adv_regex_pattern_int, items);
    ret = malloc(malloc_size);
    if (!ret)
        return NULL;

    len = strlen(pattern);

    memset(ret, 0, malloc_size);

    while(pos < len)
    {
        adv_regex_compile_recursive(&ret, pattern, len, &pos);
    }

    return ret;
}

void adv_regex_pattern_free(struct adv_regex_pattern_int *pattern)
{
    if (pattern == NULL)
        return;

    for(int c = 0; c < pattern->item_cnt; c++)
        if (pattern->items[c].type == STRING)
            free(pattern->items[c].data);

    free(pattern);
}

void *adv_regex_find(const void *pattern, int encoding, const void *start, const void *end, int *len)
{
    void *ret = NULL;

    return ret;
}

void *adv_regex_find_pattern(const char *regex, int encoding, const void *start, const void *end, int *len)
{
    void *ret = NULL;

    adv_regex_pattern pattern = adv_regex_compile(regex);
    if (pattern == NULL)
        return NULL;

    ret = adv_regex_find(pattern, encoding, start, end, len);

    adv_regex_pattern_free(pattern);

    return ret;
}
