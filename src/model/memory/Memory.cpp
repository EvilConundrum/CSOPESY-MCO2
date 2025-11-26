#pragma once
#include <vector>
#include <cstdint>
#include "../Config.cpp"
#include "./BackingStore.cpp"
#include "./Frame.cpp"

/**
 * TODO: THIS IS VERY WRONG AND NEEDS TO BE REWRITTEN LATER
 * I DO NOT WANT TO THINK ABOUT HOW DEMAND PAGING WORKS RN
 *
 * UPDATE 1: I THINK I AM COOKING
 */
class Memory
{
    std::vector<Frame> physicalMemory;
    BackingStore backingStore;

    Config *config;
    uint16_t pageSize;
    uint16_t numFrames;

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
        {
            if (frame.isValid() && frame.isDirty())
            {
                backingStore.writeRow(frame);
                frame.releaseMemory();
            }
        }
    }

    void init()
    {
        this->numFrames = this->getNumFrames(*config);
        this->physicalMemory = std::vector<Frame>(this->numFrames);

        for (int i = 0; i < this->numFrames; i++)
        {
            Frame newFrame(i, config->getMemPerFrame());
            this->physicalMemory[i] = newFrame;
        }
    }

    /**
     * Write data to a frame at given offset
     * Returns true if write was successful, false if page fault occurred
     */
    void write(int pageNumber, int offset, const uint16_t &data, uint64_t currentTick)
    {
        // check if page is in memory

        int frameIndex = handlePageFault(pageNumber, currentTick);
        if (frameIndex >= 0)
        {
            Frame &frame = physicalMemory[frameIndex];
            frame.writeData(offset, data, currentTick);
        }
    }

    uint16_t read(int pageNumber, int offset, uint64_t currentTick)
    {
        int frameIndex = handlePageFault(pageNumber, currentTick);
        if (frameIndex >= 0)
        {
            Frame &frame = physicalMemory[frameIndex];
            return frame.getData(offset, currentTick);
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
        if (frameIndex >= 0)
        {
            Frame &frame = physicalMemory[frameIndex];
            if (frame.isDirty())
                backingStore.writeRow(frame);
            frame.releaseMemory();
            return true;
        }
        return false;
    }

    /**
     * Creates or reuses a page in the backing store
     * Returns the page number to be used
     */
    int makePage() { return this->backingStore.addRow(); }

    /**
     * Marks that page as free in the backing store
     */
    void freePage(int pageNumber) { this->backingStore.freePage(pageNumber); }

    void allocateFrame()
    {
        int index = 0;
        while (index < this->getNumFrames(*config))
        {
            if (!this->physicalMemory[index].isValid())
            {
                // allocate frame here
                break;
            }
            index++;
        }
    }

    // extra functions for vmstat

    std::vector<uint8_t> getPageData(int pageNumber)
    {
        return this->backingStore.readRow(pageNumber);
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
            if (physicalMemory[i].isValid())
                if (physicalMemory[i].getPageNumber() == pageNumber)
                    index = i;
            break;
        }
        return index;
    }

    int isPageInMemory(int pageNumber) { return getFrameByPageNumber(pageNumber) != -1; }

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
        int lruIndex = 0;
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

        if (frameIndex >= 0)
            return frameIndex;
        // handle page fault

        // 1.a Check for free frame
        frameIndex = getFreeFrame();
        if (frameIndex < 0)
            // 1.b Evict a frame if all frames are occupied
            frameIndex = evictFrame();

        // 2. Read page from backing store
        std::vector<uint8_t> pageData = this->backingStore.readRow(pageNumber);

        // 3. reallocate frame
        Frame &frame = physicalMemory[frameIndex];
        frame.allocateMemory(pageNumber, currentTick);

        // 4. load data into frame
        frame.writeFromBackingStore(pageData, currentTick);

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

        Frame victimFrame = physicalMemory[frameToEvict];
        if (victimFrame.isDirty())
            backingStore.writeRow(victimFrame);
        victimFrame.releaseMemory();

        num_paged_out++;

        return frameToEvict;
    }
};