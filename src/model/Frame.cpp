#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>

class Frame
{
    int frameNumber;
    int logicalAddressStart;
    bool isValid;
    std::string currentProcessID;
    std::vector<uint8_t> data; // data is stored as bytes

public:
    Frame(int frameNum, int logAddrStart, int frameSize)
    {
        this->frameNumber = frameNum;
        this->logicalAddressStart = logAddrStart;
        this->data.resize(frameSize, 0); // initialize with zeros
    }

    /**
     * Reserves memory for a process ID
     * Returns true if successful, false if already allocated
     */
    bool reserveMemory()
    {
        if (!this->isValid)
        {
            this->isValid = true;
            return true;
        }
        return false; // frame already allocated
    }

    /**
     * Releases memory from current process (after process completes)
     * Returns true if successful, false if frame was not allocated to begin with
     */
    bool releaseMemory()
    {
        if (this->isValid)
        {
            this->isValid = false;
            return true;
        }
        return false; // frame was not allocated to begin with
    }

    /**
     * Returns entire frame data
     * will likely be unused as we already have getData
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
     * Writes uint16_t data to frame at given offset
     */
    bool writeData(int offset, const uint16_t &inputData)
    {
        // split into vector of bytes (little-endian)
        std::vector<uint8_t> bytes(2);
        bytes[0] = inputData & 0xFF;
        bytes[1] = (inputData >> 8) & 0xFF;
        return this->writeBytes(offset, bytes);
    }

private:
    bool writeBytes(int offset, const std::vector<uint8_t> &inputData)
    {
        if (offset + inputData.size() > data.size())
            return false; // page fault
        std::copy(inputData.begin(), inputData.end(), data.begin() + offset);
        return true;
    }
};