#pragma once
#include <fstream>
#include <vector>
#include <iostream>
#include "Frame.cpp"
#include "../../view/misc.cpp"

#ifdef _WIN32
#define EOL "\r\n" // Windows-readable newlines
#define EOLLEN 2
#else
// side topic: nobody in the group
//  uses linux anyway but why not
#define EOL "\n" // Unix-readable newlines
#define EOLLEN 1
#endif

/**
 * Line number = page number in backing store
 */

class BackingStore
{
    std::string filename;
    std::fstream file;
    int pageSize;
    int rowLength;

public:
    BackingStore(const std::string &name, int ps)
        : filename(name), pageSize(ps)
    {
        rowLength = pageSize * 3 - 1; // "XX " repeated pageSize times

        std::ofstream truncate(name, std::ios::trunc);
        truncate.close();

        file.open(filename,
                  std::ios::in |
                      std::ios::out |
                      std::ios::binary);

        if (!file.is_open())
            throw std::runtime_error("backing store open failed");
    }

    /**
     * Checks if page number is within range of backing store file
     * Returns true if valid, false otherwise
     */
    bool checkRange(int pageNumber)
    {
        file.clear();
        file.seekg(0, std::ios::end);
        std::streampos sz = file.tellg();
        int rows = sz / (rowLength + EOLLEN);
        return pageNumber < rows;
    }

    /**
     * Adds an empty row to the end of the backing store
     */
    void addRow()
    {
        file.clear();
        file.seekp(0, std::ios::end);

        Frame empty(-1, pageSize);
        std::string s = empty.getFrameAsString();

        file.write(s.c_str(), s.size());
        file.write(EOL, EOLLEN);
        file.flush();
    }

    /**
     * Writes a frame to the backing store at the page number specified in the frame
     * if this does returns false, it means you forgot to add a row first
     * Returns true if successful, false otherwise;
     */
    bool writeRow(Frame &page)
    {
        int pageNumber = page.getPageNumber();
        if (!checkRange(pageNumber))
            return false;

        file.clear();
        file.seekp(std::streampos(pageNumber) * (rowLength + EOLLEN),
                   std::ios::beg);

        std::string s = page.getFrameAsString();
        file.write(s.c_str(), s.size());
        file.write(EOL, EOLLEN);
        file.flush();
        return true;
    }

    /**
     * Reads a row from the backing store at given page number
     * Returns vector of uint8_t data; empty vector if out of range
     */
    std::vector<uint8_t> readRow(int n)
    {
        if (!checkRange(n))
            return {};

        file.clear();
        file.seekg(std::streampos(n) * (rowLength + EOLLEN),
                   std::ios::beg);

        std::string line;
        std::getline(file, line);

        std::vector<uint8_t> out(pageSize);
        auto parts = splitString(trimString(line), ' ');
        for (size_t i = 0; i < out.size() && i < parts.size(); ++i)
            out[i] = parseHexByte(parts[i]);

        return out;
    }
};
