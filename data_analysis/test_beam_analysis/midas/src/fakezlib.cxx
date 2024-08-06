//
// Fake libz.a for cross-compilation where cross-built libz.a is not available
//

#undef NDEBUG // midas requires assert() to be always enabled

#include <stdio.h>
#include <assert.h>
#include <zlib.h>

ZEXTERN gzFile ZEXPORT gzopen OF((const char *path, const char *mode))
{
   assert(!"fakezlib.cxx::gzopen(): libz.a is not availble in this build of MIDAS!");
}

ZEXTERN int ZEXPORT gzread OF((gzFile file, voidp buf, unsigned len))
{
   assert(!"fakezlib.cxx::gzread(): libz.a is not availble in this build of MIDAS!");
}

ZEXTERN int ZEXPORT    gzclose OF((gzFile file))
{
   assert(!"fakezlib.cxx::gzclose(): libz.a is not availble in this build of MIDAS!");
}

// end

