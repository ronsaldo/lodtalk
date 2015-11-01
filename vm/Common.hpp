#ifndef LODTALK_COMMON_HPP
#define LODTALK_COMMON_HPP

#define LODTALK_UNIMPLEMENTED() { \
	fprintf(stderr, "The method %s is unimplemented in %s at line %d\n", __PRETTY_FUNCTION__, __FILE__, __LINE__); \
	abort(); }

namespace Lodtalk
{

// Some special constants
const size_t PageSize = 4096;
const size_t PageSizeMask = PageSize - 1;
const size_t PageSizeShift = 12;

const size_t OopsPerPage = PageSize / sizeof(void*);

}

#endif // LODTALK_COMMON_HPP
