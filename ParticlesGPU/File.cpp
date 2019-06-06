#include "File.hpp"


int File::writeBinaryToFile(const char* fileName, const char* birary, size_t numBytes)
{
    FILE *output = NULL;
    fopen_s(&output, fileName, "wb");
    if(output == NULL)
        return s_Failure;

    fwrite(birary, sizeof(char), numBytes, output);
    fclose(output);

    return s_Success;
}

int File::readBinaryFromFile(const char* fileName)
{
    FILE * input = NULL;
    size_t size = 0;
    char* binary = NULL;

    fopen_s(&input, fileName, "rb");
    if(input == NULL)
    {
        return s_Failure;
    }

    fseek(input, 0L, SEEK_END); 
    size = ftell(input);
    rewind(input);
    binary = (char*)malloc(size);
    if(binary == NULL)
    {
        return s_Failure;
    }
    fread(binary, sizeof(char), size, input);
    fclose(input);
    m_Source.assign(binary, size);
    free(binary);

    return s_Success;
}

bool File::open(const char* fileName)
{
    size_t      size;
    char*       str;

    // Open file stream
    std::fstream f(fileName, (std::fstream::in | std::fstream::binary));

    // Check if we have opened file stream
    if (f.is_open()) {
        size_t  sizeFile;
        // Find the stream size
        f.seekg(0, std::fstream::end);
        size = sizeFile = (size_t)f.tellg();
        f.seekg(0, std::fstream::beg);

        str = new char[size + 1];
        if (!str) {
            f.close();
            return  false;
        }

        // Read file
        f.read(str, sizeFile);
        f.close();
        str[size] = '\0';

        m_Source  = str;
        delete[] str;

        return true;
    }
    return false;
}
