#ifndef LODTALK_CONSTANTS_HPP
#define LODTALK_CONSTANTS_HPP

#include "Lodtalk/Definitions.h"

namespace Lodtalk
{

// Some special constants
const size_t PageSize = 4096;
const size_t PageSizeMask = PageSize - 1;
const size_t PageSizeShift = 12;

const size_t OopsPerPage = PageSize / sizeof(void*);

}

#endif // LODTALK_CONSTANTS_HPP
