#pragma once
#include <string>

typedef struct LogicalAddress
{
    int pageNumber;
    int offset;
} LogicalAddress;

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

LogicalAddress parseLogicalAddress(uint16_t logicalAddress, int pageSize)
{
    return {logicalAddress / pageSize, logicalAddress % pageSize};
}

LogicalAddress parseLogicalAddress(std::string addressStr, int pageSize)
{
    int logicalAddress = std::stoi(addressStr);
    return parseLogicalAddress(logicalAddress, pageSize);
}