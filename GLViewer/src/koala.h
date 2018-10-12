#ifndef _koala_h__
#define _koala_h__

#define NAMESPACE_KOALA_ namespace koala {
#define _NAMESPACE_KOALA }

#define NAMESPACE_KPRIV_ namespace {
#define _NAMESPACE_KPRIV }

NAMESPACE_KOALA_
  typedef float float_t;
_NAMESPACE_KOALA

#include <assert.h>
#define kassert(x) assert(x);

#endif
