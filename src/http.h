#ifndef HTTP_H
#define HTTP_H
#include <stddef.h>
#include <netinet/in.h>
#include "picohttpparser.h"
#include <stdbool.h>

#define RESPONSE_BUFFER_SIZE 30 * 1024 * 1024
#define HEADER_BUFFER_SIZE 8192
#define MAX_VARIABLES 16
#define SINGLE_HEADER_SIZE 64
#define MAX_HTTP_HEADERS 128

typedef struct {
    char* ptr;
    size_t len;
    size_t size;
} Buffer;

typedef struct {
    char ptr[128];
    size_t len;
    size_t size;
} StaticVariableBuffer;

typedef struct {
    char ptr[RESPONSE_BUFFER_SIZE];
    size_t len;
    size_t size;
} StaticResponseBuffer;

typedef struct {
    char ptr[HEADER_BUFFER_SIZE];
    size_t len;
    size_t size;
} StaticHeaderBuffer;

typedef struct {
    const char* ptr;
    size_t len;
} StringView;

typedef struct {
    size_t begin;
    size_t end;
} RangeTuple;


enum HttpMethod {
    GET,
    POST,
    PUT,
    DELETE
};

enum HttpError {
    HTTP_OK,
    HTTP_PARTIAL_CONTENT,
    HTTP_NOT_FOUND,
    HTTP_BAD_REQUEST,
    HTTP_FORBIDDEN,
    HTTP_METHOD_NOT_ALLOWED,
    HTTP_CONTENT_TOO_LARGE,
    HTTP_SERVER_ERROR
};

typedef struct  {
    StaticVariableBuffer key;
    StaticVariableBuffer value;
} HttpVariable;

enum ParseStatus {
    PARSE_HEADERS = 0,
    PARSE_BODY,
    PARSE_COMPLETE
};

enum RequestStatus {
    REQUEST_PROCESSING,
    REQUEST_RESPONDING,
    REQUEST_COMPLETE
};

typedef struct {
    int fd;
    off_t offset;
    size_t length;
} FileView;

typedef struct {
    enum HttpError err;
    char content_type[64];
    bool sendfile;

    Buffer header;
    Buffer add_headers;
    Buffer body;
    FileView file;

    struct msghdr msg;
    struct iovec iov[2];    //vector of io blocks to be stitched together by 'sendmsg()'
} HttpResponse;

typedef struct HttpRequest HttpRequest;
typedef struct ContentRoute_t ContentRoute;
typedef size_t (*ContentHandler)(HttpRequest*, HttpResponse*);

struct HttpRequest{
    StringView raw_request;
    StringView path;
    StringView body;
    StringView method_str;
    size_t content_length;
    bool range_request;
    RangeTuple range;

    size_t target_output_length;
    size_t output_length;

    struct phr_header headers[100];
    size_t n_headers;
    HttpVariable variables[MAX_VARIABLES];
    size_t n_variables;

    enum HttpMethod method;
    ContentHandler content_handler;

    enum ParseStatus parse_status;
    enum RequestStatus status;

    int minor_version;
    int major_version;
    char addr[INET_ADDRSTRLEN];

};

struct ContentRoute_t {
    const char* path;
    size_t (*handler)(HttpRequest*, HttpResponse*);
};

extern const ContentRoute content_routes[];
extern const size_t num_routes;
extern const ContentHandler default_file_handler;
extern const ContentHandler default_error_handler;

HttpRequest* http_create_request(Buffer request_buffer, const char* client_address);
void http_destroy_request(HttpRequest* rq);

HttpResponse* http_create_response();
void http_destroy_response(HttpResponse* res);

void http_update_request_buffer(HttpRequest* rq, Buffer request_buffer);
void http_parse(HttpRequest* rq, HttpResponse* res);
void http_parse_body(HttpRequest* rq, HttpResponse* res);
void http_handle_request(HttpRequest* rq, HttpResponse* res);

const char* http_status_code(HttpResponse* res);

#endif
