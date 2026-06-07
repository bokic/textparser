#include "cfml.h"
#include <textparser.h>
#include <cfml_definition.json.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

// TODO: Full CFML list of cftags
const char *cfml_tags[] = {
    "cf_socialplugin",
    "cfabort",
    "cfadmin",
    "cfajaximport",
    "cfajaxproxy",
    "cfapplet",
    "cfapplication",
    "cfargument",
    "cfassociate",
    "cfauthenticate",
    "cfbreak",
    "cfcache",
    "cfcalendar",
    "cfcase",
    "cfcatch",
    "cfchart",
    "cfchartdata",
    "cfchartseries",
    "cfchartset",
    "cfclient",
    "cfclientsettings",
    "cfcol",
    "cfcollection",
    "cfcomponent",
    "cfcontent",
    "cfcontinue",
    "cfcookie",
    "cfdbinfo",
    "cfdefaultcase",
    "cfdirectory",
    "cfdiv",
    "cfdocument",
    "cfdocumentitem",
    "cfdocumentsection",
    "cfdump",
    "cfelse",
    "cfelseif",
    "cferror",
    "cfexchangecalendar",
    "cfexchangeconnection",
    "cfexchangecontact",
    "cfexchangeconversation",
    "cfexchangefilter",
    "cfexchangefolder",
    "cfexchangemail",
    "cfexchangetask",
    "cfexecute",
    "cfexit",
    "cffeed",
    "cffile",
    "cffileupload",
    "cffinally",
    "cfflush",
    "cfform",
    "cfformgroup",
    "cfformitem",
    "cfforward",
    "cfftp",
    "cffunction",
    "cfgraph",
    "cfgraphdata",
    "cfgrid",
    "cfgridcolumn",
    "cfgridrow",
    "cfgridupdate",
    "cfheader",
    "cfhtmlbody",
    "cfhtmlhead",
    "cfhtmltopdf",
    "cfhtmltopdfitem",
    "cfhttp",
    "cfhttpparam",
    "cfif",
    "cfimage",
    "cfimap",
    "cfimapfilter",
    "cfimpersonate",
    "cfimport",
    "cfinclude",
    "cfindex",
    "cfinput",
    "cfinsert",
    "cfinterface",
    "cfinvoke",
    "cfinvokeargument",
    "cfjava",
    "cflayout",
    "cflayoutarea",
    "cfldap",
    "cflocation",
    "cflock",
    "cflog",
    "cflogin",
    "cfloginuser",
    "cflogout",
    "cfloop",
    "cfmail",
    "cfmailparam",
    "cfmailpart",
    "cfmap",
    "cfmapitem",
    "cfmediaplayer",
    "cfmenu",
    "cfmenuitem",
    "cfmessagebox",
    "cfmodule",
    "cfntauthenticate",
    "cfoauth",
    "cfobject",
    "cfobjectcache",
    "cfoutput",
    "cfpageencoding",
    "cfparam",
    "cfpdf",
    "cfpdfform",
    "cfpdfformparam",
    "cfpdfparam",
    "cfpdfsubform",
    "cfpod",
    "cfpop",
    "cfpresentation",
    "cfpresentationslide",
    "cfpresenter",
    "cfprint",
    "cfprocessingdirective",
    "cfprocparam",
    "cfprocresult",
    "cfprogressbar",
    "cfproperty",
    "cfquery",
    "cfqueryparam",
    "cfregistry",
    "cfreport",
    "cfreportparam",
    "cfrethrow",
    "cfretry",
    "cfreturn",
    "cfsavecontent",
    "cfschedule",
    "cfscript",
    "cfsearch",
    "cfselect",
    "cfservlet",
    "cfservletparam",
    "cfset",
    "cfsetting",
    "cfsharepoint",
    "cfsilent",
    "cfsleep",
    "cfslider",
    "cfspreadsheet",
    "cfsprydataset",
    "cfstopwatch",
    "cfstoredproc",
    "cfswitch",
    "cftable",
    "cftextarea",
    "cftextinput",
    "cfthread",
    "cfthrow",
    "cftimer",
    "cftooltip",
    "cftrace",
    "cftransaction",
    "cftree",
    "cftreeitem",
    "cftry",
    "cfupdate",
    "cfwddx",
    "cfwebsocket",
    "cfwhile",
    "cfwindow",
    "cfxml",
    "cfzip",
    "cfzipparam"
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
            if(strncmp(tag_name, cfml_tags[i], tag_len) == 0) {
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
