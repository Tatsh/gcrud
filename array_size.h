#ifndef ARRAY_SIZE_H
#define ARRAY_SIZE_H

// Credit:
// https://zubplot.blogspot.com/2015/01/gcc-is-wonderful-better-arraysize-macro.html
#if defined(__GNUC__) && defined(__GNUC_MINOR__)
#define GNUC_VERSION (__GNUC__ << 16) + __GNUC_MINOR__
#define GNUC_PREREQ(maj, min) (GNUC_VERSION >= ((maj) << 16) + (min))
#else
#define GNUC_PREREQ(maj, min) 0
#endif
#define BUILD_BUG_ON_ZERO(e) (sizeof(struct { int : -!!(e)*1234; }))
#if GNUC_PREREQ(3, 1)
#define SAME_TYPE(a, b) __builtin_types_compatible_p(typeof(a), typeof(b))
#define MUST_BE_ARRAY(a) BUILD_BUG_ON_ZERO(SAME_TYPE((a), &(*a)))
#else
#define MUST_BE_ARRAY(a) BUILD_BUG_ON_ZERO(sizeof(a) % sizeof(*a))
#endif
#define ARRAY_SIZE(a) ((sizeof(a) / sizeof(*a)) + MUST_BE_ARRAY(a))

#endif
