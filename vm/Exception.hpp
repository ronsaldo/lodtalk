#ifndef LODTALK_EXCEPTION_HPP
#define LODTALK_EXCEPTION_HPP

#include <stdexcept>

namespace Lodtalk
{
/**
 * Native exception
 */
class NativeException: public std::exception
{
};

/**
 * Native error
 */
class NativeError: public NativeException
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

void nativeErrorFormat(const char *format, ...);

inline void nativeSubclassResponsibility()
{
	nativeError("Subclass responsibility.");
}

inline void nativeNotYetImplemented()
{
	nativeError("Subclass not yet implemented.");
}

} // End of namespace Lodtalk

#endif //LODTALK_EXCEPTION_HPP
