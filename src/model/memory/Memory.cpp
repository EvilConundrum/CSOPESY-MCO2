#pragma once
#include <vector>
#include <cstdint>
#include "../Config.cpp"
#include "./Frame.cpp"

/**
 * TODO: THIS IS VERY WRONG AND NEEDS TO BE REWRITTEN LATER
 * I DO NOT WANT TO THINK ABOUT HOW DEMAND PAGING WORKS RN
 * 
 * UPDATE: I SORTA UNDERSTAND WHAT IT DOES BUT IT DOES MAKE MY HEAD EXPLODE
 */
class Memory
{
    std::vector<Frame> physicalMemory;
    // this would contain frames to be swapped in and out from physical memory
    // TODO: LRU or FIFO to be implemented in this class
    std::vector<Frame> virtualMemory;
    Config *config;
    int pageSize;
    int pageCtr = 0; // for assigning page numbers when a new process is created

public:
    Memory() {}

    Memory(Config *config)
    {
        this->config = config;
        this->pageSize = config->getMemPerFrame();
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

    void allocateFrame()
    {
        // To Be Implemented: implement virtual memory swapping here
    }

    Frame fetchFrame(int frameNumber)
    {
        // To Be Implemented: implement virtual memory swapping here
        return this->physicalMemory[frameNumber];
    }

    // TODO: make an interface for memory management

private:
    int getNumFrames(const Config &config)
    {
        return config.getMaxOverallMem() / config.getMemPerFrame();
    }
};