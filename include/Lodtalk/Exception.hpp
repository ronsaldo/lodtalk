#ifndef LODTALK_EXCEPTION_HPP
#define LODTALK_EXCEPTION_HPP

#include <stdexcept>
#include "Lodtalk/Definitions.h"

#ifdef _MSC_VER
#  pragma warning( push )
#  pragma warning( disable: 4251 )
#  pragma warning( disable: 4275 )
#endif

namespace Lodtalk
{
/**
 * Native exception
 */
class LODTALK_VM_EXPORT NativeException: public std::exception
{
};

/**
 * Native error
 */
class LODTALK_VM_EXPORT NativeError: public NativeException
{
public:
	NativeError(const std::string &errorMessage) noexcept
		: errorMessage(errorMessage) {}

	virtual const char* what() const noexcept
	{
		return errorMessage.c_str();
	}

private:
	std::string errorMessage;
};

inline void nativeError(const std::string &what)
{
	throw NativeError(what);
}

LODTALK_VM_EXPORT void nativeErrorFormat(const char *format, ...);

inline void nativeSubclassResponsibility()
{
	nativeError("Subclass responsibility.");
}

inline void nativeNotYetImplemented()
{
	nativeError("Subclass not yet implemented.");
}

} // End of namespace Lodtalk

#ifdef _MSC_VER
#  pragma warning( pop )
#endif

#endif //LODTALK_EXCEPTION_HPP
