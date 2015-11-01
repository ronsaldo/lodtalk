#include "FileSystem.hpp"

namespace Lodtalk
{
	
std::string dirname(const std::string &filename)
{
	size_t forwardSlashEnd = filename.rfind('/');
	size_t backwardSlashEnd = filename.rfind('\\');
	size_t lastFolder = std::string::npos;
	if(forwardSlashEnd != std::string::npos && backwardSlashEnd != std::string::npos)
		lastFolder = std::max(forwardSlashEnd, backwardSlashEnd);
	else if(forwardSlashEnd != std::string::npos)
		lastFolder = forwardSlashEnd;
	else if(backwardSlashEnd != std::string::npos)
		lastFolder = backwardSlashEnd;
		
	if(lastFolder != std::string::npos)
		return filename.substr(0, lastFolder + 1);
	else
		return std::string();
}

std::string joinPath(const std::string &path1, const std::string &path2)
{
	if(isAbsolutePath(path2))
		return path2;

	if(path1.empty())
		return path2;
	if(path2.empty())
		return path1;
		
	if(path1.back() == '/' || path1.back() == '\\')
		return path1 + path2;
#ifdef _WIN32
	return path1 + "\\" + path2;
#else
	return path1 + "/" + path2;
#endif
}

bool isAbsolutePath(const std::string &path)
{
#ifdef _WIN32
	return path.size() >= 2 && path[1] == ':';
#else
	return !path.empty() && path[0] == '/';
#endif
}

}