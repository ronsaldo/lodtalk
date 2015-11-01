#ifndef LODTALK_RAII_HPP
#define LODTALK_RAII_HPP

#include <string.h>
#include <stdio.h>

namespace Lodtalk
{

/**
 * C Standard library file destructor.
 */	
class StdFile
{
public:
	StdFile(const std::string &filename, const char *openMode)
		: file(fopen(filename.c_str(), openMode)) {}

	StdFile(FILE *file)
		: file(file) {}

		
	~StdFile()
	{
		if(file)
			fclose(file);
	}
	
	FILE *get()
	{
		return file;
	}
	
	operator FILE*()
	{
		return file;
	}
	
private:
	FILE *file;
};
} // End of namespace

#endif //LODTALK_RAII_HPP