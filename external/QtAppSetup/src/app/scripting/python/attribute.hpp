#pragma once

#if __GNUC__ >= 4
#   define PY_HIDDEN  __attribute__ ((visibility ("hidden")))
#else
#   define PY_HIDDEN
#endif
