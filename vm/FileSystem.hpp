#ifndef LODTALK_FILESYSTEM_HPP
#define LODTALK_FILESYSTEM_HPP

#include <string>

namespace Lodtalk
{

std::string dirname(const std::string &filename);
std::string joinPath(const std::string &path1, const std::string &path2);
bool isAbsolutePath(const std::string &path);

} // End of namespace Lodtalk

#endif //LODTALK_FILESYSTEM_HPP