#include <stdarg.h>
#include <stdio.h>
#include "Lodtalk/Exception.hpp"

namespace Lodtalk
{

void nativeErrorFormat(const char *format, ...)
{
    char buffer[1024];
    va_list args;
    va_start(args, format);
    vsnprintf(buffer, 1024, format, args);
	va_end(args);

	nativeError(buffer);
}

} // End of namespace Lodtalk
