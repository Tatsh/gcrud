#ifndef GCRUD_WHITELIST_H
#define GCRUD_WHITELIST_H

extern gboolean whitelist_check(const char *ce) __attribute__((nonnull));
extern void whitelist_cleanup();

#endif // GCRUD_WHITELIST_H
