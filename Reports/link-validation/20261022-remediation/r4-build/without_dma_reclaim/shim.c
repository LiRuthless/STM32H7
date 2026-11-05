#include <stddef.h>
typedef void (*reporter_t)(const char *, int);
static reporter_t reporter;
void test_set_reporter(reporter_t value) { reporter=value; }
void test_fail(const char *expr) { reporter(expr,1); for (;;) {} }
int puts(const char *message) { reporter(message,0); return 0; }
void *memset(void *dest,int value,size_t size)
{ unsigned char *p=dest; for(size_t i=0;i<size;++i)p[i]=(unsigned char)value; return dest; }
