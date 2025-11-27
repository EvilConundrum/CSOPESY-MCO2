#include <iostream>
#include <vector>
#include <cstdint>
#include "../src/model/memory/BackingStore.cpp"
#include "../src/model/memory/Frame.cpp"
#include "../src/model/Config.cpp"
#include "../src/model/memory/LogicalAddress.cpp"
#include "../src/model/memory/Memory.cpp"

void memtest()
{
    std::string backingStoreFilename = "backing_store_test.txt";
    Config config("test-config.txt");
    Memory memory(&config, backingStoreFilename);

    /**
     * Test Case:
     * 64 bytes / 16 bytes per frame = 4 possible frames in physical memory
     * Available pages for this test case: 0 to 7
     * Sequence of pages: 0, 1, 2, 3, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 1, 2, 3, 4, 5, 6, 7
     */

    // initialize the backing store
    memory.makePages(1 << 8);
    
    std::vector<int> pageSequence = {0, 1, 2, 3, 1, 2, 3, 4, 5, 6, 7, 0, 1, 2, 3, 1, 2, 3, 4, 5, 6, 7};

    for (int tick = 0; tick < pageSequence.size(); tick++)
    {
        int pageNumber = pageSequence[tick];
        std::cout << "Tick " << tick << ": writing to page " << pageNumber << std::endl;
        memory.write(pageNumber, (tick * 2) % config.getMemPerFrame(), static_cast<uint16_t>(pageNumber + 100), tick);
    }

    std::cout << "Hits: " << memory.getNumHits() << " Faults: " << memory.getNumFaults() << std::endl;

    // read pages

    for (int tick = 0; tick < pageSequence.size(); tick++)
    {
        int pageNumber = pageSequence[tick];
        uint16_t data = memory.read(pageNumber, (tick * 2) % config.getMemPerFrame(), tick);
        std::cout << "Tick " << tick << ": read from page " << pageNumber << " data: " << data << std::endl;
    }
}

void writeTestConfig()
{
    std::string filepath = "test-config.txt";
    std::string content = "num-cpu 1\nscheduler \"rr\"\nquantum-cycles 5\nbatch-process-freq 1\nmin-ins 1\nmax-ins 5\ndelay-per-exec 0\nmax-overall-mem 64\nmem-per-frame 16\nmin-mem-per-proc 1024\nmax-mem-per-proc 32768\n";

    std::ofstream configFile(filepath, std::ios::trunc);
    configFile << content;
    configFile.close();
}

void deleteTestConfig()
{
    std::string filepath = "test-config.txt";
    if (std::remove(filepath.c_str()) != 0)
    {
        std::perror("Error deleting test config file");
    }
}

int main()
{
    writeTestConfig();
    memtest();
    // deleteTestConfig();
    return 0;
}