#pragma once
#include <vector>
#include <cstdint>
#include "../model/Config.cpp"

class Frame
{
    int frameNumber;
    int logicalAddressStart;
    std::vector<uint8_t> data; // data is stored as bytes

public:
    Frame(int frameNum, int logAddrStart, int frameSize)
    {
        this->frameNumber = frameNum;
        this->logicalAddressStart = logAddrStart;
        this->data.resize(frameSize, 0); // initialize with zeros
    }

    /**
     * Returns entire frame data
     */
    std::vector<uint8_t> getFrame() const
    {
        return data;
    }

    std::string getFrameAsString() const
    {
        std::string result;
        std::stringstream ss;
        int i = 0;
        for (const auto &byte : data)
        {

            if (byte < 16)
                ss << "0"; // leading zero for single digit hex

            ss << std::hex << std::uppercase << static_cast<int>(byte);

            if (i < data.size() - 1)
                if ((i + 1) % 16 == 0)
                    ss << "\n";
                else
                    ss << " ";

            i++;

            result += ss.str();

            ss.str("");
        }
        return result;
    }

    /**
     * Returns uint16_t data from frame at given offset and length
     */
    uint16_t getData(int offset) const
    {
        if (offset + 2 > data.size())
            return 0; // return 0 if out of bounds

        // return little-endian uint16_t
        return (data[offset] | (data[offset + 1] << 8));
    }

    /**
     * Writes data to frame at given offset
     */
    bool writeData(int offset, const std::vector<uint8_t> &inputData)
    {
        if (offset + inputData.size() > data.size())
            return false; // page fault
        std::copy(inputData.begin(), inputData.end(), data.begin() + offset);
        return true;
    }

    /**
     * Writes uint16_t data to frame at given offset
     */
    bool writeData(int offset, const uint16_t &inputData)
    {
        // split into vector of bytes (little-endian)
        std::vector<uint8_t> bytes(2);
        bytes[0] = inputData & 0xFF;
        bytes[1] = (inputData >> 8) & 0xFF;
        return this->writeData(offset, bytes);
    }
};

/**
 * TODO: THIS IS VERY WRONG AND NEEDS TO BE REWRITTEN LATER
 * I DO NOT WANT TO THINK ABOUT HOW DEMAND PAGING WORKS RN
 */
class Memory
{
    std::vector<Frame> physicalMemory;
    std::vector<Frame> virtualMemory;
    Config *config;
    Memory() {}

    Memory(Config *config)
    {
        this->config = config;
        this->init();
    }

    void init()
    {
        int numFrames = this->getNumFrames(*config);
        for (int i = 0; i < numFrames; i++)
        {
            Frame newFrame(i, i * config->getMemPerFrame(), config->getMemPerFrame());
            this->physicalMemory.push_back(newFrame);
        }
    }

    /**
     * Returns true if write was successful, false if page fault occurred
     */
    bool write(int frameNumber, int offset, const uint16_t &data)
    {
        if (frameNumber < 0 || frameNumber >= physicalMemory.size())
            return false; // page fault
        return true;
    }

    uint16_t fetchFrame(int frameNumber, int offset)
    {
        // To Be Implemented
        return 0;
    }

private:
    int getNumFrames(const Config &config)
    {
        return config.getMaxOverallMem() / config.getMemPerFrame();
    }
};