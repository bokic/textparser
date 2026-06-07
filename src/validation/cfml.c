#include "cfml.h"
#include <textparser.h>
#include <cfml_definition.json.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

// TODO: Full CFML list of cftags
typedef enum {
    CFML_END_TAG_REQUIRED, // must have ending tag </...
    CFML_END_TAG_OPTIONAL, // can have ending tag, or can be self-closing
    CFML_END_TAG_FORBIDDEN // should not have ending tag
} cfml_end_tag_type;

typedef struct {
    const char *name;
    cfml_end_tag_type end_tag_type;
} cfml_tag_info;

const cfml_tag_info cfml_tags[] = {
    {"cf_socialplugin", CFML_END_TAG_OPTIONAL},
    {"cfabort", CFML_END_TAG_FORBIDDEN},
    {"cfadmin", CFML_END_TAG_FORBIDDEN},
    {"cfajaximport", CFML_END_TAG_FORBIDDEN},
    {"cfajaxproxy", CFML_END_TAG_FORBIDDEN},
    {"cfapplet", CFML_END_TAG_FORBIDDEN},
    {"cfapplication", CFML_END_TAG_OPTIONAL},
    {"cfargument", CFML_END_TAG_FORBIDDEN},
    {"cfassociate", CFML_END_TAG_FORBIDDEN},
    {"cfauthenticate", CFML_END_TAG_FORBIDDEN},
    {"cfbreak", CFML_END_TAG_FORBIDDEN},
    {"cfcache", CFML_END_TAG_FORBIDDEN},
    {"cfcalendar", CFML_END_TAG_FORBIDDEN},
    {"cfcase", CFML_END_TAG_REQUIRED},
    {"cfcatch", CFML_END_TAG_REQUIRED},
    {"cfchart", CFML_END_TAG_REQUIRED},
    {"cfchartdata", CFML_END_TAG_FORBIDDEN},
    {"cfchartseries", CFML_END_TAG_OPTIONAL},
    {"cfchartset", CFML_END_TAG_REQUIRED},
    {"cfclient", CFML_END_TAG_REQUIRED},
    {"cfclientsettings", CFML_END_TAG_FORBIDDEN},
    {"cfcol", CFML_END_TAG_FORBIDDEN},
    {"cfcollection", CFML_END_TAG_FORBIDDEN},
    {"cfcomponent", CFML_END_TAG_REQUIRED},
    {"cfcontent", CFML_END_TAG_FORBIDDEN},
    {"cfcontinue", CFML_END_TAG_FORBIDDEN},
    {"cfcookie", CFML_END_TAG_FORBIDDEN},
    {"cfdbinfo", CFML_END_TAG_FORBIDDEN},
    {"cfdefaultcase", CFML_END_TAG_REQUIRED},
    {"cfdirectory", CFML_END_TAG_FORBIDDEN},
    {"cfdiv", CFML_END_TAG_OPTIONAL},
    {"cfdocument", CFML_END_TAG_REQUIRED},
    {"cfdocumentitem", CFML_END_TAG_REQUIRED},
    {"cfdocumentsection", CFML_END_TAG_REQUIRED},
    {"cfdump", CFML_END_TAG_FORBIDDEN},
    {"cfelse", CFML_END_TAG_FORBIDDEN},
    {"cfelseif", CFML_END_TAG_FORBIDDEN},
    {"cferror", CFML_END_TAG_FORBIDDEN},
    {"cfexchangecalendar", CFML_END_TAG_FORBIDDEN},
    {"cfexchangeconnection", CFML_END_TAG_FORBIDDEN},
    {"cfexchangecontact", CFML_END_TAG_FORBIDDEN},
    {"cfexchangeconversation", CFML_END_TAG_FORBIDDEN},
    {"cfexchangefilter", CFML_END_TAG_FORBIDDEN},
    {"cfexchangefolder", CFML_END_TAG_FORBIDDEN},
    {"cfexchangemail", CFML_END_TAG_FORBIDDEN},
    {"cfexchangetask", CFML_END_TAG_FORBIDDEN},
    {"cfexecute", CFML_END_TAG_OPTIONAL},
    {"cfexit", CFML_END_TAG_FORBIDDEN},
    {"cffeed", CFML_END_TAG_FORBIDDEN},
    {"cffile", CFML_END_TAG_FORBIDDEN},
    {"cffileupload", CFML_END_TAG_FORBIDDEN},
    {"cffinally", CFML_END_TAG_REQUIRED},
    {"cfflush", CFML_END_TAG_FORBIDDEN},
    {"cfform", CFML_END_TAG_REQUIRED},
    {"cfformgroup", CFML_END_TAG_REQUIRED},
    {"cfformitem", CFML_END_TAG_OPTIONAL},
    {"cfforward", CFML_END_TAG_FORBIDDEN},
    {"cfftp", CFML_END_TAG_FORBIDDEN},
    {"cffunction", CFML_END_TAG_REQUIRED},
    {"cfgraph", CFML_END_TAG_OPTIONAL},
    {"cfgraphdata", CFML_END_TAG_FORBIDDEN},
    {"cfgrid", CFML_END_TAG_OPTIONAL},
    {"cfgridcolumn", CFML_END_TAG_FORBIDDEN},
    {"cfgridrow", CFML_END_TAG_FORBIDDEN},
    {"cfgridupdate", CFML_END_TAG_FORBIDDEN},
    {"cfheader", CFML_END_TAG_FORBIDDEN},
    {"cfhtmlbody", CFML_END_TAG_OPTIONAL},
    {"cfhtmlhead", CFML_END_TAG_OPTIONAL},
    {"cfhtmltopdf", CFML_END_TAG_REQUIRED},
    {"cfhtmltopdfitem", CFML_END_TAG_REQUIRED},
    {"cfhttp", CFML_END_TAG_OPTIONAL},
    {"cfhttpparam", CFML_END_TAG_FORBIDDEN},
    {"cfif", CFML_END_TAG_REQUIRED},
    {"cfimage", CFML_END_TAG_FORBIDDEN},
    {"cfimap", CFML_END_TAG_FORBIDDEN},
    {"cfimapfilter", CFML_END_TAG_FORBIDDEN},
    {"cfimpersonate", CFML_END_TAG_REQUIRED},
    {"cfimport", CFML_END_TAG_FORBIDDEN},
    {"cfinclude", CFML_END_TAG_FORBIDDEN},
    {"cfindex", CFML_END_TAG_FORBIDDEN},
    {"cfinput", CFML_END_TAG_FORBIDDEN},
    {"cfinsert", CFML_END_TAG_FORBIDDEN},
    {"cfinterface", CFML_END_TAG_REQUIRED},
    {"cfinvoke", CFML_END_TAG_OPTIONAL},
    {"cfinvokeargument", CFML_END_TAG_FORBIDDEN},
    {"cfjava", CFML_END_TAG_REQUIRED},
    {"cflayout", CFML_END_TAG_REQUIRED},
    {"cflayoutarea", CFML_END_TAG_REQUIRED},
    {"cfldap", CFML_END_TAG_FORBIDDEN},
    {"cflocation", CFML_END_TAG_FORBIDDEN},
    {"cflock", CFML_END_TAG_REQUIRED},
    {"cflog", CFML_END_TAG_FORBIDDEN},
    {"cflogin", CFML_END_TAG_REQUIRED},
    {"cfloginuser", CFML_END_TAG_FORBIDDEN},
    {"cflogout", CFML_END_TAG_FORBIDDEN},
    {"cfloop", CFML_END_TAG_REQUIRED},
    {"cfmail", CFML_END_TAG_REQUIRED},
    {"cfmailparam", CFML_END_TAG_FORBIDDEN},
    {"cfmailpart", CFML_END_TAG_REQUIRED},
    {"cfmap", CFML_END_TAG_OPTIONAL},
    {"cfmapitem", CFML_END_TAG_FORBIDDEN},
    {"cfmediaplayer", CFML_END_TAG_OPTIONAL},
    {"cfmenu", CFML_END_TAG_REQUIRED},
    {"cfmenuitem", CFML_END_TAG_FORBIDDEN},
    {"cfmessagebox", CFML_END_TAG_FORBIDDEN},
    {"cfmodule", CFML_END_TAG_OPTIONAL},
    {"cfntauthenticate", CFML_END_TAG_FORBIDDEN},
    {"cfoauth", CFML_END_TAG_FORBIDDEN},
    {"cfobject", CFML_END_TAG_FORBIDDEN},
    {"cfobjectcache", CFML_END_TAG_FORBIDDEN},
    {"cfoutput", CFML_END_TAG_REQUIRED},
    {"cfpageencoding", CFML_END_TAG_FORBIDDEN},
    {"cfparam", CFML_END_TAG_FORBIDDEN},
    {"cfpdf", CFML_END_TAG_OPTIONAL},
    {"cfpdfform", CFML_END_TAG_OPTIONAL},
    {"cfpdfformparam", CFML_END_TAG_FORBIDDEN},
    {"cfpdfparam", CFML_END_TAG_FORBIDDEN},
    {"cfpdfsubform", CFML_END_TAG_OPTIONAL},
    {"cfpod", CFML_END_TAG_OPTIONAL},
    {"cfpop", CFML_END_TAG_FORBIDDEN},
    {"cfpresentation", CFML_END_TAG_REQUIRED},
    {"cfpresentationslide", CFML_END_TAG_REQUIRED},
    {"cfpresenter", CFML_END_TAG_FORBIDDEN},
    {"cfprint", CFML_END_TAG_FORBIDDEN},
    {"cfprocessingdirective", CFML_END_TAG_OPTIONAL},
    {"cfprocparam", CFML_END_TAG_FORBIDDEN},
    {"cfprocresult", CFML_END_TAG_FORBIDDEN},
    {"cfprogressbar", CFML_END_TAG_FORBIDDEN},
    {"cfproperty", CFML_END_TAG_FORBIDDEN},
    {"cfquery", CFML_END_TAG_REQUIRED},
    {"cfqueryparam", CFML_END_TAG_FORBIDDEN},
    {"cfregistry", CFML_END_TAG_FORBIDDEN},
    {"cfreport", CFML_END_TAG_OPTIONAL},
    {"cfreportparam", CFML_END_TAG_FORBIDDEN},
    {"cfrethrow", CFML_END_TAG_FORBIDDEN},
    {"cfretry", CFML_END_TAG_FORBIDDEN},
    {"cfreturn", CFML_END_TAG_FORBIDDEN},
    {"cfsavecontent", CFML_END_TAG_REQUIRED},
    {"cfschedule", CFML_END_TAG_FORBIDDEN},
    {"cfscript", CFML_END_TAG_REQUIRED},
    {"cfsearch", CFML_END_TAG_FORBIDDEN},
    {"cfselect", CFML_END_TAG_OPTIONAL},
    {"cfservlet", CFML_END_TAG_OPTIONAL},
    {"cfservletparam", CFML_END_TAG_FORBIDDEN},
    {"cfset", CFML_END_TAG_FORBIDDEN},
    {"cfsetting", CFML_END_TAG_FORBIDDEN},
    {"cfsharepoint", CFML_END_TAG_OPTIONAL},
    {"cfsilent", CFML_END_TAG_REQUIRED},
    {"cfsleep", CFML_END_TAG_FORBIDDEN},
    {"cfslider", CFML_END_TAG_FORBIDDEN},
    {"cfspreadsheet", CFML_END_TAG_FORBIDDEN},
    {"cfsprydataset", CFML_END_TAG_FORBIDDEN},
    {"cfstopwatch", CFML_END_TAG_REQUIRED},
    {"cfstoredproc", CFML_END_TAG_OPTIONAL},
    {"cfswitch", CFML_END_TAG_REQUIRED},
    {"cftable", CFML_END_TAG_REQUIRED},
    {"cftextarea", CFML_END_TAG_OPTIONAL},
    {"cftextinput", CFML_END_TAG_FORBIDDEN},
    {"cfthread", CFML_END_TAG_REQUIRED},
    {"cfthrow", CFML_END_TAG_FORBIDDEN},
    {"cftimer", CFML_END_TAG_REQUIRED},
    {"cftooltip", CFML_END_TAG_REQUIRED},
    {"cftrace", CFML_END_TAG_OPTIONAL},
    {"cftransaction", CFML_END_TAG_REQUIRED},
    {"cftree", CFML_END_TAG_REQUIRED},
    {"cftreeitem", CFML_END_TAG_FORBIDDEN},
    {"cftry", CFML_END_TAG_REQUIRED},
    {"cfupdate", CFML_END_TAG_FORBIDDEN},
    {"cfwddx", CFML_END_TAG_FORBIDDEN},
    {"cfwebsocket", CFML_END_TAG_OPTIONAL},
    {"cfwhile", CFML_END_TAG_REQUIRED},
    {"cfwindow", CFML_END_TAG_OPTIONAL},
    {"cfxml", CFML_END_TAG_REQUIRED},
    {"cfzip", CFML_END_TAG_OPTIONAL},
    {"cfzipparam", CFML_END_TAG_FORBIDDEN}
};
const int cfml_tag_count = sizeof(cfml_tags) / sizeof(cfml_tags[0]);

static char *dynamic_printf(const char *format, ...) {
    va_list args1, args2;

    // 1. Initialize the variable argument lists
    va_start(args1, format);
    // We make a copy because a va_list can only be read through once safely
    va_copy(args2, args1);

    // 2. Ask vsnprintf how many characters the final string will need
    // We pass NULL and 0 so it only counts the length without writing anything
    int length = vsnprintf(NULL, 0, format, args1);
    va_end(args1);

    // If something went wrong with formatting, return NULL
    if (length < 0) {
        va_end(args2);
        return NULL;
    }

    // 3. Allocate memory (+1 is vital to leave room for the '\0' null terminator)
    char *buffer = malloc(length + 1);
    if (buffer == NULL) {
        va_end(args2);
        return NULL; // Return NULL if the system runs out of memory
    }

    // 4. Fill the allocated buffer with our formatted text
    vsnprintf(buffer, length + 1, format, args2);
    va_end(args2);

    // 5. Return the newly created string to the caller
    return buffer;
}

static void textparser_validation_item_add(enum textparser_validation_item_type type, textparser_validation **validation, char *text, int position, int length) {
    if (*validation == nullptr) {
        *validation = malloc(offsetof(textparser_validation, items));
        (*validation)->len = 0;
    } else {
        *validation = realloc(*validation, offsetof(textparser_validation, items) + sizeof(textparser_validation_item) * ((*validation)->len + 1));
    }

    (*validation)->len++;
    (*validation)->items[(*validation)->len - 1] = malloc(sizeof(textparser_validation_item));
    (*validation)->items[(*validation)->len - 1]->type = type;
    (*validation)->items[(*validation)->len - 1]->position = position;
    (*validation)->items[(*validation)->len - 1]->length = length;
    (*validation)->items[(*validation)->len - 1]->text = text;
}

void textparser_validation_clear(textparser_validation *validation)
{
    if (validation != nullptr) {
        for (int i = 0; i < validation->len; i++) {
            free(validation->items[i]->text);
            free(validation->items[i]);
        }
        free(validation);
    }
}

static void textparser_validate_cfml_token(textparser_validation **ret, textparser_t handle, textparser_token_item *token)
{
    const char *text = textparser_get_text(handle);
    //size_t text_size = textparser_get_text_size(handle);

    // Check if token is tag
    if(token->token_id == TextParser_cfml_StartTag) {

        // TODO: Check if tag exists in the cfml_tags list
        int tag_exists = 0;

        // Get tag name and length
        const char *tag_name = text + token->position + 1;
        size_t tag_len = token->len - 1;
        for(size_t i = 0; i < tag_len; i++) {
            if (
                (tag_name[i] < 'A' || tag_name[i] > 'Z') &&
                (tag_name[i] < 'a' || tag_name[i] > 'z') &&
                (tag_name[i] < '0' || tag_name[i] > '9')
            ) {
                tag_len = i;
                break;
            }
        }

        // Check if tag exists in the cfml_tags list
        for(int i = 0; i < cfml_tag_count; i++) {
            if(strncmp(tag_name, cfml_tags[i].name, tag_len) == 0) {
                tag_exists = 1;
                break;
            }
        }
        if(!tag_exists) {
            char *token_name = alloca(tag_len + 1);
            strncpy(token_name, tag_name, tag_len);
            token_name[tag_len] = '\0';
            char *str = dynamic_printf("Unknown CFML tag: [%s]", token_name);

            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, tag_len);
            return;
        }

        // TODO: Check if needs closing tag

        // TODO: Check if is opening tag, and it has correct attributes/expression
    }

    // TODO: Check if end tag has it's own corresponding start tag

    // TODO: Check if token is function
    // Check if function has correct arguments, and return value
}

enum textparser_encoding textparser_get_encoding_cfml(textparser_t handle) {
    // https://helpx.adobe.com/coldfusion/cfml-reference/coldfusion-tags/tags-p-q/cfprocessingdirective.html
    // Parse first 4096 bytes of the file to check for cfprocessingdirective cf tag


    const char *text = textparser_get_text(handle);


    //const char *text_addr;
    //size_t text_size;

    return TEXTPARSER_ENCODING_UNICODE;
}

textparser_validation *textparser_validate_cfml(textparser_t handle) {
    textparser_validation *ret = nullptr;
    textparser_token_item *token = nullptr;

    token = textparser_get_first_token(handle);
    while (token != nullptr) {

        textparser_validate_cfml_token(&ret, handle, token);

        token = token->next;
    }

    return ret;
}
