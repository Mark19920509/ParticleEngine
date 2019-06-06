#ifndef SDKFILE_HPP_
#define SDKFILE_HPP_

#include <vector>
#include <string>
#include <fstream>
#include <malloc.h>

class File
{
public:
    File(): m_Source(""){}
    ~File(){};

    //
	//Opens the CL program file
	//@return true if success else false
	//
    bool open(const char* fileName);

	//
	//writeBinaryToFile
	//@param fileName Name of the file
	//@param binary char binary array
	//@param numBytes number of bytes
	//@return true if success else false
	//
    int writeBinaryToFile(const char* fileName, const char* binary, size_t numBytes);

	//
	//readBinaryToFile
	//@param fileName name of file
	//@return true if success else false
	//
    int readBinaryFromFile(const char* fileName);

    
	
	
	//Returns a pointer to the string object with the source code
    const std::string&  source() const { return m_Source; }

private:
    //
	//Disable copy constructor
	//
    File(const File&);

    //
	//Disable operator=
	//
    File& operator=(const File&);

    std::string     m_Source;

    static const int s_Failure = 1;
    static const int s_Success = 0;
};


#endif  // SDKFile_HPP_
