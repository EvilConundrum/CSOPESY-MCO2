#pragma once
#include <string>

typedef struct LogicalAddress
{
    int pageNumber;
    int offSet;
} LogicalAddress;

uint16_t convHexToUint16(const std::string &hexStr)
{
    std::string hexStrCopy = hexStr;
    if (hexStrCopy.rfind("0x", 0) == 0 || hexStrCopy.rfind("0X", 0) == 0)
        hexStrCopy = hexStrCopy.substr(2);

    return static_cast<uint16_t>(std::stoul(hexStrCopy, nullptr, 16));
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