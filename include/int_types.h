#pragma once

// inttypes.h appears to be unavailable so we define some useful things ourselves

#ifndef PRIx64
# if __WORDSIZE == 64
#  define __PRI64_PREFIX	"l"
# else
#  define __PRI64_PREFIX	"ll" // NOLINT(*-reserved-identifier)
# endif
# define PRIx64		__PRI64_PREFIX "x"
#endif

#if !defined(PRIu32) or !defined(PRIx32)
# if __WORDSIZE == 64
#  define __PRI32_PREFIX	""
#else
#  define __PRI32_PREFIX	"l"
#endif
#endif

#ifndef PRIu32
# define PRIu32		__PRI32_PREFIX "u"
#endif

#ifndef PRIx32
# define PRIx32		__PRI32_PREFIX "u"
#endif

#ifndef PRIb32
# define PRIb32		__PRI32_PREFIX "b"
#endif
