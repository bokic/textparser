#include "cfml.h"
#include <textparser.h>
#include <cfml_definition.json.h>

#include <string.h>
#include <malloc.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>


typedef enum {
    CFML_END_TAG_REQUIRED, // must have ending tag </...
    CFML_END_TAG_OPTIONAL, // can have ending tag, or can be self-closing
    CFML_END_TAG_FORBIDDEN // should not have ending tag
} cfml_end_tag_type;

typedef enum {
    CFML_PARAMS_NONE,        // cftags that do not have any parameters
    CFML_PARAMS_KEY_VALUE,   // e.g. <cfmail to="foo">
    CFML_PARAMS_EXPRESSION   // e.g. <cfif condition>, <cfset x = 1>
} cfml_params_type;

typedef enum {
    CFML_ATTR_TYPE_ANY,
    CFML_ATTR_TYPE_STRING,
    CFML_ATTR_TYPE_NUMBER,
    CFML_ATTR_TYPE_BOOLEAN
} cfml_attr_type;

typedef enum {
    CFML_MASK_LITERAL = 1 << 0,
    CFML_MASK_VARIABLE = 1 << 1,
    CFML_MASK_EXPRESSION = 1 << 2,
    CFML_MASK_ALL = CFML_MASK_LITERAL | CFML_MASK_VARIABLE | CFML_MASK_EXPRESSION
} cfml_mask_type;

typedef struct {
    const char *name;
    bool required;
    cfml_attr_type type;
    const char *value;
    int mask_kind;
} cfml_attribute_info;

typedef struct {
    const cfml_attribute_info *attributes;
} cfml_attribute_combo;

typedef struct {
    const char *name;
    cfml_end_tag_type end_tag_type;
    cfml_params_type params_type;
    const cfml_attribute_combo *attribute_combos;
} cfml_tag_info;

static const cfml_attribute_info cfloop_index_attrs[] = {
    {"index", true, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_VARIABLE},
    {"from", true, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {"to", true, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {"step", false, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_info cfloop_cond_attrs[] = {
    {"condition", true, CFML_ATTR_TYPE_ANY, nullptr, CFML_MASK_EXPRESSION},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_info cfloop_query_attrs[] = {
    {"query", true, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"startrow", false, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {"endrow", false, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {"group", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"groupcasesensitive", false, CFML_ATTR_TYPE_BOOLEAN, nullptr, CFML_MASK_ALL},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_info cfloop_list_attrs[] = {
    {"list", true, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"index", true, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_VARIABLE},
    {"delimiters", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_info cfloop_array_attrs[] = {
    {"array", true, CFML_ATTR_TYPE_ANY, nullptr, CFML_MASK_ALL},
    {"index", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_VARIABLE},
    {"item", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_VARIABLE},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_info cfloop_collection_attrs[] = {
    {"collection", true, CFML_ATTR_TYPE_ANY, nullptr, CFML_MASK_ALL},
    {"item", true, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_VARIABLE},
    {"index", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_VARIABLE},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_info cfloop_file_attrs[] = {
    {"file", true, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"index", true, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_VARIABLE},
    {"charset", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"characters", false, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_combo cfloop_combos[] = {
    {cfloop_index_attrs},
    {cfloop_cond_attrs},
    {cfloop_query_attrs},
    {cfloop_list_attrs},
    {cfloop_array_attrs},
    {cfloop_collection_attrs},
    {cfloop_file_attrs},
    {nullptr}
};

static const cfml_attribute_info cfabort_attrs[] = {
    {"showerror", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_combo cfabort_combos[] = {
    {cfabort_attrs},
    {nullptr}
};

static const cfml_attribute_info cfinclude_attrs[] = {
    {"template", true, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"runonce", false, CFML_ATTR_TYPE_BOOLEAN, nullptr, CFML_MASK_ALL},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_combo cfinclude_combos[] = {
    {cfinclude_attrs},
    {nullptr}
};

static const cfml_attribute_info cfparam_attrs[] = {
    {"name", true, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"type", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"default", false, CFML_ATTR_TYPE_ANY, nullptr, CFML_MASK_ALL},
    {"max", false, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {"min", false, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {"pattern", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_combo cfparam_combos[] = {
    {cfparam_attrs},
    {nullptr}
};

static const cfml_attribute_info cfmail_attrs[] = {
    {"to", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"from", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"subject", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"cc", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"bcc", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"type", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"charset", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"server", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"port", false, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {"username", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"password", false, CFML_ATTR_TYPE_STRING, nullptr, CFML_MASK_ALL},
    {"timeout", false, CFML_ATTR_TYPE_NUMBER, nullptr, CFML_MASK_ALL},
    {nullptr, false, CFML_ATTR_TYPE_ANY, nullptr, 0}
};

static const cfml_attribute_combo cfmail_combos[] = {
    {cfmail_attrs},
    {nullptr}
};

const cfml_tag_info cfml_tags[] = {
    {"cf_socialplugin", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfabort", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, cfabort_combos},
    {"cfadmin", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfajaximport", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfajaxproxy", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfapplet", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfapplication", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfargument", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfassociate", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfauthenticate", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfbreak", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_NONE, nullptr},
    {"cfcache", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfcalendar", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfcase", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfcatch", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfchart", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfchartdata", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfchartseries", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfchartset", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfclient", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfclientsettings", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfcol", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfcollection", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfcomponent", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfcontent", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfcontinue", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_NONE, nullptr},
    {"cfcookie", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfdbinfo", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfdefaultcase", CFML_END_TAG_REQUIRED, CFML_PARAMS_NONE, nullptr},
    {"cfdirectory", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfdiv", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfdocument", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfdocumentitem", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfdocumentsection", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfdump", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfelse", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_NONE, nullptr},
    {"cfelseif", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_EXPRESSION, nullptr},
    {"cferror", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexchangecalendar", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexchangeconnection", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexchangecontact", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexchangeconversation", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexchangefilter", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexchangefolder", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexchangemail", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexchangetask", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexecute", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfexit", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cffeed", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cffile", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cffileupload", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cffinally", CFML_END_TAG_REQUIRED, CFML_PARAMS_NONE, nullptr},
    {"cfflush", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfform", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfformgroup", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfformitem", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfforward", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfftp", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cffunction", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfgraph", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfgraphdata", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfgrid", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfgridcolumn", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfgridrow", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfgridupdate", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfheader", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfhtmlbody", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfhtmlhead", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfhtmltopdf", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfhtmltopdfitem", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfhttp", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfhttpparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfif", CFML_END_TAG_REQUIRED, CFML_PARAMS_EXPRESSION, nullptr},
    {"cfimage", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfimap", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfimapfilter", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfimpersonate", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfimport", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfinclude", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, cfinclude_combos},
    {"cfindex", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfinput", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfinsert", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfinterface", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfinvoke", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfinvokeargument", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfjava", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cflayout", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cflayoutarea", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfldap", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cflocation", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cflock", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cflog", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cflogin", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfloginuser", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cflogout", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_NONE, nullptr},
    {"cfloop", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, cfloop_combos},
    {"cfmail", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, cfmail_combos},
    {"cfmailparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfmailpart", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfmap", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfmapitem", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfmediaplayer", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfmenu", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfmenuitem", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfmessagebox", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfmodule", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfntauthenticate", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfoauth", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfobject", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfobjectcache", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_NONE, nullptr},
    {"cfoutput", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpageencoding", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, cfparam_combos},
    {"cfpdf", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpdfform", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpdfformparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpdfparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpdfsubform", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpod", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpop", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpresentation", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpresentationslide", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfpresenter", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfprint", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfprocessingdirective", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfprocparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfprocresult", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfprogressbar", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfproperty", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfquery", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfqueryparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfregistry", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfreport", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfreportparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfrethrow", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_NONE, nullptr},
    {"cfretry", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_NONE, nullptr},
    {"cfreturn", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_EXPRESSION, nullptr},
    {"cfsavecontent", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfschedule", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfscript", CFML_END_TAG_REQUIRED, CFML_PARAMS_NONE, nullptr},
    {"cfsearch", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfselect", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfservlet", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfservletparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfset", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_EXPRESSION, nullptr},
    {"cfsetting", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfsharepoint", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfsilent", CFML_END_TAG_REQUIRED, CFML_PARAMS_NONE, nullptr},
    {"cfsleep", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfslider", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfspreadsheet", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfsprydataset", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfstopwatch", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfstoredproc", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfswitch", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftable", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftextarea", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftextinput", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfthread", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfthrow", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftimer", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftooltip", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftrace", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftransaction", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftree", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftreeitem", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cftry", CFML_END_TAG_REQUIRED, CFML_PARAMS_NONE, nullptr},
    {"cfupdate", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfwddx", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfwebsocket", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfwhile", CFML_END_TAG_REQUIRED, CFML_PARAMS_EXPRESSION, nullptr},
    {"cfwindow", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfxml", CFML_END_TAG_REQUIRED, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfzip", CFML_END_TAG_OPTIONAL, CFML_PARAMS_KEY_VALUE, nullptr},
    {"cfzipparam", CFML_END_TAG_FORBIDDEN, CFML_PARAMS_KEY_VALUE, nullptr}
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

static bool is_start_tag_token(int token_id) {
    return token_id == TextParser_cfml_StartTag ||
           token_id == TextParser_cfml_LoopStartTag ||
           token_id == TextParser_cfml_QueryStartTag ||
           token_id == TextParser_cfml_OutputStartTag ||
           token_id == TextParser_cfml_ScriptStartTag;
}

static bool is_end_tag_token(int token_id) {
    return token_id == TextParser_cfml_EndTag ||
           token_id == TextParser_cfml_LoopEndTag ||
           token_id == TextParser_cfml_QueryEndTag ||
           token_id == TextParser_cfml_OutputEndTag ||
           token_id == TextParser_cfml_ScriptEndTag;
}

static bool is_token_self_closing(const char *text, textparser_token_item *token) {
    if (token->len >= 2) {
        const char *end = text + token->position + token->len;
        while (end > text + token->position && (end[-1] == ' ' || end[-1] == '\t' || end[-1] == '\r' || end[-1] == '\n')) {
            end--;
        }
        if (end - (text + token->position) >= 2 && end[-2] == '/' && end[-1] == '>') {
            return true;
        }
    }
    return false;
}

static const cfml_tag_info *find_tag_info(const char *tag_name, size_t tag_len) {
    for (int i = 0; i < cfml_tag_count; i++) {
        if (strncmp(tag_name, cfml_tags[i].name, tag_len) == 0 && cfml_tags[i].name[tag_len] == '\0') {
            return &cfml_tags[i];
        }
    }
    return nullptr;
}

static bool has_matching_end_tag(const char *text, textparser_token_item *start_token, const char *tag_name, size_t tag_len) {
    int nesting = 1;
    textparser_token_item *curr = start_token->next;
    while (curr != nullptr) {
        if (is_start_tag_token(curr->token_id)) {
            const char *curr_name = text + curr->position + 1;
            size_t curr_len = 0;
            while (curr_name[curr_len] && 
                   ((curr_name[curr_len] >= 'a' && curr_name[curr_len] <= 'z') ||
                    (curr_name[curr_len] >= 'A' && curr_name[curr_len] <= 'Z') ||
                    (curr_name[curr_len] >= '0' && curr_name[curr_len] <= '9') ||
                    curr_name[curr_len] == '_')) {
                curr_len++;
            }
            if (curr_len == tag_len && strncmp(curr_name, tag_name, tag_len) == 0) {
                if (!is_token_self_closing(text, curr)) {
                    nesting++;
                }
            }
        }
        else if (is_end_tag_token(curr->token_id)) {
            const char *curr_name = text + curr->position + 2;
            size_t curr_len = 0;
            while (curr_name[curr_len] && 
                   ((curr_name[curr_len] >= 'a' && curr_name[curr_len] <= 'z') ||
                    (curr_name[curr_len] >= 'A' && curr_name[curr_len] <= 'Z') ||
                    (curr_name[curr_len] >= '0' && curr_name[curr_len] <= '9') ||
                    curr_name[curr_len] == '_')) {
                curr_len++;
            }
            if (curr_len == tag_len && strncmp(curr_name, tag_name, tag_len) == 0) {
                nesting--;
                if (nesting == 0) {
                    return true;
                }
            }
        }
        curr = curr->next;
    }
    return false;
}

static bool has_matching_start_tag(const char *text, textparser_token_item *end_token, const char *tag_name, size_t tag_len) {
    int nesting = 1;
    textparser_token_item *curr = end_token->prev;
    while (curr != nullptr) {
        if (is_end_tag_token(curr->token_id)) {
            const char *curr_name = text + curr->position + 2;
            size_t curr_len = 0;
            while (curr_name[curr_len] && 
                   ((curr_name[curr_len] >= 'a' && curr_name[curr_len] <= 'z') ||
                    (curr_name[curr_len] >= 'A' && curr_name[curr_len] <= 'Z') ||
                    (curr_name[curr_len] >= '0' && curr_name[curr_len] <= '9') ||
                    curr_name[curr_len] == '_')) {
                curr_len++;
            }
            if (curr_len == tag_len && strncmp(curr_name, tag_name, tag_len) == 0) {
                nesting++;
            }
        }
        else if (is_start_tag_token(curr->token_id)) {
            const char *curr_name = text + curr->position + 1;
            size_t curr_len = 0;
            while (curr_name[curr_len] && 
                   ((curr_name[curr_len] >= 'a' && curr_name[curr_len] <= 'z') ||
                    (curr_name[curr_len] >= 'A' && curr_name[curr_len] <= 'Z') ||
                    (curr_name[curr_len] >= '0' && curr_name[curr_len] <= '9') ||
                    curr_name[curr_len] == '_')) {
                curr_len++;
            }
            if (curr_len == tag_len && strncmp(curr_name, tag_name, tag_len) == 0) {
                if (!is_token_self_closing(text, curr)) {
                    nesting--;
                    if (nesting == 0) {
                        return true;
                    }
                }
            }
        }
        curr = curr->prev;
    }
    return false;
}

static void textparser_validate_cfml_token(textparser_validation **ret, textparser_t handle, textparser_token_item *token)
{
    const char *text = textparser_get_text(handle);

    if (is_start_tag_token(token->token_id)) {
        // Get tag name and length
        const char *tag_name = text + token->position + 1;
        size_t tag_len = 0;
        while (tag_name[tag_len] && 
               ((tag_name[tag_len] >= 'a' && tag_name[tag_len] <= 'z') ||
                (tag_name[tag_len] >= 'A' && tag_name[tag_len] <= 'Z') ||
                (tag_name[tag_len] >= '0' && tag_name[tag_len] <= '9') ||
                tag_name[tag_len] == '_')) {
            tag_len++;
        }

        // Check if tag exists in the cfml_tags list
        const cfml_tag_info *info = find_tag_info(tag_name, tag_len);
        if (info == nullptr) {
            char *token_name = alloca(tag_len + 1);
            strncpy(token_name, tag_name, tag_len);
            token_name[tag_len] = '\0';
            char *str = dynamic_printf("Unknown CFML tag: [%s]", token_name);

            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, tag_len);
            return;
        }

        // Check if needs closing tag
        if (info->end_tag_type == CFML_END_TAG_REQUIRED) {
            if (!is_token_self_closing(text, token)) {
                if (!has_matching_end_tag(text, token, tag_name, tag_len)) {
                    char *token_name = alloca(tag_len + 1);
                    strncpy(token_name, tag_name, tag_len);
                    token_name[tag_len] = '\0';
                    char *str = dynamic_printf("CFML tag [%s] requires a closing tag </%s>", token_name, token_name);
                    textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
                }
            }
        }
    }
    else if (is_end_tag_token(token->token_id)) {
        // Get tag name and length (skipping </)
        const char *tag_name = text + token->position + 2;
        size_t tag_len = 0;
        while (tag_name[tag_len] && 
               ((tag_name[tag_len] >= 'a' && tag_name[tag_len] <= 'z') ||
                (tag_name[tag_len] >= 'A' && tag_name[tag_len] <= 'Z') ||
                (tag_name[tag_len] >= '0' && tag_name[tag_len] <= '9') ||
                tag_name[tag_len] == '_')) {
            tag_len++;
        }

        const cfml_tag_info *info = find_tag_info(tag_name, tag_len);
        if (info != nullptr) {
            // Check if ending tag is forbidden
            if (info->end_tag_type == CFML_END_TAG_FORBIDDEN) {
                char *token_name = alloca(tag_len + 1);
                strncpy(token_name, tag_name, tag_len);
                token_name[tag_len] = '\0';
                char *str = dynamic_printf("Ending tag </%s> is forbidden", token_name);
                textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
                return;
            }
        }

        // Check if end tag has its own corresponding start tag
        if (!has_matching_start_tag(text, token, tag_name, tag_len)) {
            char *token_name = alloca(tag_len + 1);
            strncpy(token_name, tag_name, tag_len);
            token_name[tag_len] = '\0';
            char *str = dynamic_printf("Ending tag </%s> has no matching start tag", token_name);
            textparser_validation_item_add(TEXTPARSER_VALIDATION_ITEM_TYPE_ERROR, ret, str, token->position, token->len);
        }
    }
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
