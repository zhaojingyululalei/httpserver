#ifndef _http11_common_h
#define _http11_common_h

#include <sys/types.h>
//data是HttpRequestParser   at是解析完成的结果
typedef void (*element_cb)(void *data, const char *at, size_t length);
//data是HttpRequestParser   filed是key  value是value
typedef void (*field_cb)(void *data, const char *field, size_t flen, const char *value, size_t vlen);

#endif
