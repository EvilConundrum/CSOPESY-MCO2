#pragma once
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include "../../view/misc.cpp"

class Frame
{
    int frameNumber; // this never changes after initialization
    int pageNumber; // page number of the process currently occupying this frame
    std::string currentProcessID; // empty string indicates free frame
    uint64_t lastAccessTick;
    std::vector<uint8_t> data; // data is stored as bytes
public:
    Frame(int frameNum, int frameSize)
    {
        this->lastAccessTick = 0;
        this->frameNumber = frameNum;
        this->data.resize(frameSize, 0); // initialize with zeros
    }

    // Getters
    int getFrameNumber() const { return this->frameNumber; }
    int getPageNumber() const { return this->pageNumber; }
    std::string getCurrentProcessID() const { return this->currentProcessID; }

    // Setters
    void updateLastAccessTick(uint64_t tick) { this->lastAccessTick = tick; }

    /**
     * Returns true if frame is currently allocated to a process
     */
    bool isValid() const { return !this->currentProcessID.empty() || this->lastAccessTick != 0; }

    /**
     * Reserves memory for a process ID
     * Returns true if successful, false if already allocated
     */
    bool allocateMemory(int pageNumber, std::string processID, uint64_t currentTick)
    {
        if (!this->isValid())
        {
            this->currentProcessID = processID;
            this->pageNumber = pageNumber;
            this->updateLastAccessTick(currentTick);
            return true;
        }
        return false; // frame already allocated
    }

    /**
     * Releases memory from current process (after process completes)
     * Returns true if successful, false if frame was not allocated to begin with
     */
    bool releaseMemory(uint64_t currentTick)
    {
        if (this->isValid())
        {
            this->currentProcessID = "";
            this->lastAccessTick = 0;
            std::fill(this->data.begin(), this->data.end(), 0);
            return true;
        }
        return false; // frame was not allocated to begin with
    }

    /**
     * Returns entire frame data
     * is used for writing to backing store
     */
    std::vector<uint8_t> getFrame() const
    {
        return data;
    }

    std::string getFrameAsString(int numPerRow = -1) const
    {
        std::string result;
        std::stringstream ss;
        int i = 0;
        for (const auto &byte : data)
        {
            bool addNewline = (++i % numPerRow == 0 && numPerRow > 0);
            result += formatByte(byte);

            if (i < data.size())
                result += addNewline ? "\n" : " ";
            ss.str("");
        }
        return result;
    }

    /**
     * Returns uint16_t data from frame at given offset and length
     */
    uint16_t getData(int offset, uint64_t currentTick)
    {
        if (offset + 2 > data.size() || offset < 0)
            return 0; // return 0 if out of bounds

        this->updateLastAccessTick(currentTick);

        // return little-endian uint16_t
        return (data[offset] | (data[offset + 1] << 8));
    }

    /**
     * Writes uint16_t data to frame at given offset
     */
    bool writeData(int offset, const uint16_t &inputData, uint64_t currentTick)
    {
        // split into vector of bytes (little-endian)
        std::vector<uint8_t> bytes(2);
        bytes[0] = inputData & 0xFF;
        bytes[1] = (inputData >> 8) & 0xFF;
        bool success = this->writeBytes(offset, bytes);
        if (success)
            this->updateLastAccessTick(currentTick);

        return success;
    }

    void writeFromBackingStore(const std::vector<uint8_t> &inputData, uint64_t currentTick)
    {
        // assumed to be of correct size since called from Memory manager
        this->writeBytes(0, inputData);
        this->updateLastAccessTick(currentTick);
    }

private:
    bool writeBytes(int offset, const std::vector<uint8_t> &inputData)
    {
        if (offset + inputData.size() > data.size() || offset < 0)
            return false; // page fault
        std::copy(inputData.begin(), inputData.end(), data.begin() + offset);
        return true;
    }
};