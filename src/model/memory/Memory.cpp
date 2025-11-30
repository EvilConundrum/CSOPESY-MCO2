#pragma once
#include "../../headers.h"
#include <vector>
#include <cstdint>
#include "../Config.cpp"
#include "./BackingStore.cpp"
#include "./Frame.cpp"
#include "./Address.cpp"

/**
 * Memory management unit, handles page reads/writes and page faults
 */
class Memory
{
    std::vector<Frame> physicalMemory;
    BackingStore backingStore;

    Config *config;
    uint16_t pageSize;
    uint16_t numFrames;

    uint16_t num_hits;
    uint16_t num_faults;

    uint64_t num_paged_in;
    uint64_t num_paged_out;

public:
    Memory() {}

    Memory(Config *config, std::string backingStoreFilename)
    {
        this->config = config;
        this->backingStore = BackingStore(backingStoreFilename, config);
        this->pageSize = config->getMemPerFrame();
        this->init();
    }

    ~Memory()
    {
        // save all allocated frames to backing store
        for (auto &frame : physicalMemory)
            frame.releaseMemory();
    }

    void init()
    {
        this->numFrames = this->getNumFrames(*config);
        this->physicalMemory = std::vector<Frame>();

        for (int i = 0; i < this->numFrames; i++)
        {
            Frame newFrame(i, config->getMemPerFrame());
            this->physicalMemory.push_back(newFrame);
        }
    }

    uint16_t getPageSize() { return this->pageSize; }

    /**
     * Write data to a frame at given offset
     * Returns true if write was successful, false if page fault occurred
     */
    void write(LogicalAddress address, const uint16_t &data, uint64_t currentTick)
    {
        // check if page is in memory
        int frameIndex = handlePageFault(address.pageNumber, currentTick);

        if (frameIndex >= 0)
        {
            Frame &frame = physicalMemory[frameIndex];
            frame.writeData(address.offset, data, currentTick);
        }
    }

    /**
     * Reads data from a frame at given offset
     * Handles page faults if page is not in memory
     */
    uint16_t read(LogicalAddress address, uint64_t currentTick)
    {
        int frameIndex = handlePageFault(address.pageNumber, currentTick);
        if (frameIndex >= 0)
        {
            Frame &frame = physicalMemory[frameIndex];
            return frame.getData(address.offset, currentTick);
        }

        return 0; // should not reach here
    }

    /**
     * Releases a page from memory
     * is called when a process terminates
     * Returns true if successful, false if page was not found
     */
    bool releasePage(int pageNumber)
    {
        int frameIndex = getFrameByPageNumber(pageNumber);

        backingStore.freePage(pageNumber);

        if (frameIndex >= 0)
        {
            // if (physicalMemory[frameIndex].isDirty())
            //     backingStore.writeRow(physicalMemory[frameIndex]);
            physicalMemory[frameIndex].releaseMemory();
            return true;
        }

        return false;
    }

    std::vector<int> makePages(int memNeeded)
    {
        int pagesNeeded = memNeeded / pageSize;
        if (memNeeded % pageSize != 0)
            pagesNeeded++;

        std::vector<int> pageNumbers;
        for (int i = 0; i < pagesNeeded; i++)
            pageNumbers.push_back(this->backingStore.addRow());

        return pageNumbers;
    }
    
    std::string getMemoryState() const
    {
        std::string result;
        for (const auto &frame : physicalMemory)
        {
            result += "Frame " + std::to_string(frame.getFrameNumber()) + ": ";
            if (frame.isValid())
            {
                result += "Page " + std::to_string(frame.getPageNumber()) + ", Last Access Tick: " + std::to_string(frame.getLastAccessTick()) + "\n";
            }
            else
            {
                result += "Free\n";
            }
        }
        return result;
    }

    std::string getMemorySnapshot() const
    {
        std::string result;

        int numFrameDigits = std::to_string(this->physicalMemory.size()).length();
        // gets the number of digits in the highest page number
        int numPageDigits;

        for (const auto &frame : physicalMemory)
        {
            if (frame.isValid())
            {
                int pageNumDigits = std::to_string(frame.getPageNumber()).length();
                if (pageNumDigits > numPageDigits)
                    numPageDigits = pageNumDigits;
            }
        }

        auto padLeft = [](const std::string &s, int totalLength, char paddingChar = ' ')
        {
            if (s.length() >= totalLength)
                return s;
            return std::string(totalLength - s.length(), paddingChar) + s;
        };

        numPageDigits = std::max(numPageDigits, 4); // minimum 4 digits for page numbers

        int i = 0;
        for (const auto &frame : physicalMemory)
        {
            result += "[" + padLeft(std::to_string(frame.getFrameNumber()), numFrameDigits) + "] ";
            result += "Page: " + (padLeft(frame.isValid() ? std::to_string(frame.getPageNumber()) : "Free", numPageDigits)) + " Data: ";
            result += frame.getFrameAsString() + "\n";
        }
        return result;
    }

    bool flushAllPagesToBackingStore()
    {
        for (auto &frame : physicalMemory)
        {
            if (frame.isValid() && frame.isDirty())
                if (!backingStore.writeRow(frame))
                    return false;
        }
        return true;
    }

    // extra functions for vmstat and debugging

    std::vector<uint8_t> getPageData(int pageNumber) { return this->backingStore.readRow(pageNumber); }

    uint64_t getNumPagedIn() const { return num_paged_in; }
    uint64_t getNumPagedOut() const { return num_paged_out; }

    uint64_t getNumHits() const { return num_hits; }
    uint64_t getNumFaults() const { return num_faults; }

    uint64_t getTotalMemoryBytes() const { return numFrames * pageSize; }
    uint64_t getFreeMemoryBytes() const {
        int freeFrames = 0;
        for (const auto &frame : physicalMemory)
        {
            if (!frame.isValid())
                freeFrames++;
        }
        return freeFrames * pageSize;
    }
    uint64_t getUsedMemoryBytes() const { return getTotalMemoryBytes() - getFreeMemoryBytes(); }

    uint64_t getMemUsedByProcess(const std::vector<int> &pageNumbers, int memoryAllocated)
    {
        uint64_t usedBytes = 0;
        for (int pageNumber : pageNumbers)
        {
            if (this->isPageInMemory(pageNumber))
                usedBytes += pageSize;
        }

        // if all pages are in memory, cap usedBytes to memoryAllocated as the last page may not be fully used
        if (usedBytes > memoryAllocated)
            usedBytes = memoryAllocated;

        return usedBytes;
    }

private:
    int getNumFrames(const Config &config)
    {
        return config.getMaxOverallMem() / config.getMemPerFrame();
    }

    /**
     * returns the index of the frame containing the given page number
     * if not found, returns -1
     */
    int getFrameByPageNumber(int pageNumber)
    {
        int index = -1;
        for (int i = 0; i < physicalMemory.size() && index == -1; i++)
        {
            int framePageNumber = physicalMemory[i].getPageNumber();
            if (framePageNumber >= 0 && framePageNumber == pageNumber)
                index = i;
        }

        return index;
    }

    bool isPageInMemory(int pageNumber) { return getFrameByPageNumber(pageNumber) != -1; }

    /**
     * Returns the first free frame index found
     * if no free frame, returns -1
     */
    int getFreeFrame()
    {
        int index = -1;
        for (int i = 0; i < physicalMemory.size() && index == -1; i++)
        {
            if (!physicalMemory[i].isValid())
                index = i;
        }
        return index;
    }

    /**
     * returns the index of the frame to be evicted
     * assumes that there is no free frame available
     */
    int getLRUFrame()
    {
        int lruIndex = -1;
        uint64_t lruTick = UINT64_MAX;

        for (int i = 0; i < physicalMemory.size(); i++)
        {
            if (physicalMemory[i].isValid() && physicalMemory[i].getLastAccessTick() < lruTick)
            {
                lruTick = physicalMemory[i].getLastAccessTick();
                lruIndex = i;
            }
        }

        return lruIndex;
    }

    /**
     * checks and handles page fault for given page number
     * returns index of frame containing the page after handling fault
     */
    int handlePageFault(int pageNumber, int currentTick)
    {
        int frameIndex = getFrameByPageNumber(pageNumber);

        // page hit
        if (frameIndex >= 0)
        {
            ++num_hits;
            return frameIndex;
        }
        ++num_faults;

        // handle page fault

        // 1.a Check for free frame
        frameIndex = getFreeFrame();
        if (frameIndex < 0)
            // 1.b Evict a frame if all frames are occupied
            frameIndex = evictFrame();

        // 2. Read page from backing store
        std::vector<uint8_t> pageData = this->backingStore.readRow(pageNumber);

        // 3. reallocate frame
        physicalMemory[frameIndex].allocateMemory(pageNumber, currentTick);

        // 4. load data into frame
        physicalMemory[frameIndex].writeFromBackingStore(pageData, currentTick);

        // 5. update stats
        ++num_paged_in;

        return frameIndex;
    }

    /**
     * Evicts a frame using LRU policy
     * Writes the frame back to backing store
     * returns the index of the freed frame
     */
    int evictFrame()
    {
        int frameToEvict = getLRUFrame();

        if (physicalMemory[frameToEvict].isDirty())
            backingStore.writeRow(physicalMemory[frameToEvict]);
        physicalMemory[frameToEvict].releaseMemory();

        num_paged_out++;

        return frameToEvict;
    }
};