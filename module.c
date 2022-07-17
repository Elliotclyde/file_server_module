#include <apr.h>
#include <apr_pools.h>
#include <apr_file_io.h>
#include <httpd.h>
#include <http_protocol.h>
#include <http_config.h>

typedef struct
{
    const char *docRoot;
} read_config;

static read_config config;

const char *set_docroot(cmd_parms *cmd, void *cfg, const char *arg)
{
    config.docRoot = arg;
    return NULL;
}

static const command_rec read_directives[] =
    {
        AP_INIT_TAKE1("DocumentRoot", set_docroot, NULL, RSRC_CONF, "Document root"),
        {NULL}};

char *extension_to_mimetype(char *fileExtension)
{
    if (strcmp(fileExtension, "html") == 0)
    {
        return "text/html;charset=utf8";
    }
    if (strcmp(fileExtension, "txt") == 0)
    {
        return "text/plain;charset=utf8";
    }
    if (strcmp(fileExtension, "css") == 0)
    {
        return "text/css;charset=utf8";
    }
    if (strcmp(fileExtension, "js") == 0)
    {
        return "text/js;charset=utf8";
    }
    if (strcmp(fileExtension, "jpg") == 0 || strcmp(fileExtension, "jpeg") == 0)
    {
        return "image/jpeg;";
    }
    if (strcmp(fileExtension, "gif") == 0)
    {
        return "image/gif;";
    }
    if (strcmp(fileExtension, "png") == 0)
    {
        return "image/png;";
    }
    if (strcmp(fileExtension, "svg") == 0)
    {
        return "image/svg;";
    }
    return "text/html;charset=utf8";
}

static int readfile_apache_handler(request_rec *r)
{

    if (!r->handler || (strcmp(r->handler, "apache_readfile_module") != 0))
    {
        return DECLINED;
    }
    if (r->method_number != M_GET)
    {
        return HTTP_METHOD_NOT_ALLOWED;
    }

    int status = OK;

    char *relativeFilePath = NULL;
    relativeFilePath = strdup(r->uri);
    if (relativeFilePath[strlen(relativeFilePath) - 1] == '/')
    {
        strcat(relativeFilePath, "index.html");
    }

    char *absoluteFilePath = NULL;
    absoluteFilePath = strdup(config.docRoot);
    strcat(absoluteFilePath, relativeFilePath);

    char *dot = strrchr(absoluteFilePath, '.');
    char *fileExtension = dot + 1;

    for (int i = 0; i < strlen(absoluteFilePath); i++)
    {
        if (absoluteFilePath[i] == '/')
        {
            absoluteFilePath[i] = '\\';
        }
    }

    apr_file_t *file;

    apr_status_t fileStatus = apr_file_open(&file, absoluteFilePath, APR_FOPEN_READ, APR_OS_DEFAULT, r->pool);
    if (fileStatus != APR_SUCCESS)
    {
        status = HTTP_NOT_FOUND;
        ap_rputs("<h1>Could not open file</h1>", r);
    }
    else
    {
        apr_finfo_t fileInfo;

        apr_file_info_get(&fileInfo, APR_FINFO_SIZE, file);

        apr_off_t size = fileInfo.size;

        int bufferSize = size + 1;
        char *buffer = apr_palloc(r->pool, bufferSize);
        memset(buffer, 0, bufferSize);

        apr_size_t sizeOfBuffer = bufferSize;

        fileStatus = apr_file_read(file, buffer, &sizeOfBuffer);
        if (fileStatus != APR_SUCCESS)
        {
            status = HTTP_INTERNAL_SERVER_ERROR;
            ap_rputs("<h1>Could not read file</h1>", r);
        }
        else
        {
            ap_set_content_type(r, extension_to_mimetype(fileExtension));
            ap_rputs(buffer, r);
        }
    }
    return status;
}

static void read_file_register_hooks(apr_pool_t *pool)
{
    ap_hook_handler(readfile_apache_handler, NULL, NULL, APR_HOOK_MIDDLE);
}

module AP_MODULE_DECLARE_DATA apache_readfile_module = {
    STANDARD20_MODULE_STUFF,
    NULL,
    NULL,
    NULL,
    NULL,
    read_directives,
    read_file_register_hooks};
