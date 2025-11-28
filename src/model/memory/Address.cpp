#pragma once
#include <string>

typedef struct Address
{
    int pageNumber; // virtual page number (physical needs to be translated using allocatedPages)
    int offset;     // offset within the page (virtual and physical share the same offset)
    std::string str() {
        return "[" + std::to_string(pageNumber) + ", " + std::to_string(offset) + "]";
    };
} Address;

// Inheritance approach
typedef struct LogicalAddress : public Address {} LogicalAddress;
typedef struct VirtualAddress : public Address {} VirtualAddress;

/**
 * Converts a hexadecimal string (e.g., "0x1A3F") to its uint16_t representation
 * requires hex string to start with "0x" or "0X" to prevent misinterpretation with decimal
 */
uint16_t convHexToUint16(const std::string &hexStr)
{
    std::string hexStrCopy = hexStr;
    if (hexStrCopy.rfind("0x", 0) == 0 || hexStrCopy.rfind("0X", 0) == 0)
        hexStrCopy = hexStrCopy.substr(2);
    else
        return 0; // not a valid hex string

    try
    {
        return static_cast<uint16_t>(std::stoul(hexStrCopy, nullptr, 16));
    }
    catch (const std::exception &e)
    {
        throw e;
    }
}

uint16_t makeLogicalAddress(LogicalAddress addr, int pageSize)
{
    return static_cast<uint16_t>(addr.pageNumber * pageSize + addr.offset);
}
/**
 * Resolves the address into virtual page number and offset
 */
VirtualAddress parseVirtualAddress(uint16_t virtualAddress, int pageSize)
{
    return {virtualAddress / pageSize, virtualAddress % pageSize};
}

/**
 * Resolves the address into virtual page number and offset
 */
VirtualAddress parseVirtualAddress(std::string addressStr, int pageSize)
{
    int virtualAddress = std::stoi(addressStr);
    return parseVirtualAddress(virtualAddress, pageSize);
}