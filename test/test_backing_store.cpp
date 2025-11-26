#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include "../src/model/memory/BackingStore.cpp"
#include "../src/model/memory/Frame.cpp"

void test_free()
{
    Config config("test-config.txt");
    BackingStore backingStore("backing_store_test_write.txt", &config);

    Frame frame(0, config.getMemPerFrame());
    int freedPage = 7;
    frame.allocateMemory(freedPage, 0);

    int pageSize = config.getMemPerFrame(); // bytes per page

    for (int i = 0; i <= 16; i++)
    {
        backingStore.addRow();
    }

    // write uint16_t values
    for (int j = 0; j < pageSize; j += 2)
        frame.writeData(j, static_cast<uint16_t>((j + 1) * (freedPage + 1)), freedPage * 2);

    // free page using frame
    backingStore.freePage(freedPage);
    frame.releaseMemory();

    // free page 4
    backingStore.freePage(4);
    
    int newPage = backingStore.addRow();
    std::cout << frame.allocateMemory(newPage, 0) << std::endl; // reallocate to check if data cleared

    std::cout << "Reused page number: " << newPage << std::endl;
    for (int j = 0; j < pageSize; j += 2)
    {
        frame.writeData(j, static_cast<uint16_t>(67), newPage * 2);
    }
    std::cout << frame.getPageNumber() << std::endl;
    backingStore.writeRow(frame);

    std::vector<uint8_t> data = backingStore.readRow(newPage);
    std::cout << "Data at reused page " << newPage << ": ";
    for (int i = 0; i < data.size(); i++)
    {
        std::cout << formatByte(data[i]) << " ";
    }
    std::cout << std::endl;
}

void test_write()
{
    int pageSize = 16; // bytes per page

    Config config("test-config.txt");
    BackingStore backingStore("backing_store_test_write.txt", &config);

    int curr_page = 0;
    std::vector<Frame> frames;

    for (int i = 0; i < 16; i++)
    {
        Frame frame(i, pageSize);
        frame.allocateMemory(i, i * 2);

        for (int j = 0; j < pageSize; j += 2)
            frame.writeData(j, static_cast<uint16_t>((j + 1) * (i + 1)), i * 2); // write uint16_t values

        frames.push_back(frame);
    }

    bool writeSuccess = true;
    for (int i = 0; i < frames.size() && writeSuccess; i++)
    {
        Frame &frame = frames[i];
        backingStore.addRow();
        if (!backingStore.writeRow(const_cast<Frame &>(frame)))
            writeSuccess = false;
    }

    if (writeSuccess)
        std::cout << "Write successful." << std::endl;
    else
        std::cout << "Write failed." << std::endl;

    std::cout << std::endl
              << std::endl;
}

void test_read()
{
    int pageSize = 16; // bytes per page

    Config config("test-config.txt");
    BackingStore backingStore("backing_store_test_write.txt", &config);

    // add rows
    for (int i = 0; i < 16; i++)
        backingStore.addRow();

    // Write multiple frames
    std::vector<Frame> frames;
    for (int pageNumber = 0; pageNumber < 10; pageNumber++)
    {
        Frame frame(pageNumber, pageSize);
        frame.allocateMemory(pageNumber, pageNumber * 10);

        for (int i = 0; i < pageSize; i += 2)
            frame.writeData(i, static_cast<uint16_t>((i + 1) * (pageNumber + 1)), pageNumber * 10);

        frames.push_back(frame);
        backingStore.writeRow(frame);
        std::cout << "Written page " << pageNumber << ": " << frame.getFrameAsString() << std::endl;
    }

    std::cout << "\n--- Reading back data ---\n"
              << std::endl;

    // Read multiple frames and verify
    bool allMatch = true;
    for (int pageNumber = 0; pageNumber < 10; pageNumber++)
    {
        std::vector<uint8_t> data = backingStore.readRow(pageNumber);

        if (data.empty())
        {
            std::cout << "Read failed for page " << pageNumber << std::endl;
            allMatch = false;
            continue;
        }

        // Check if read data matches what was written
        bool match = true;
        for (int i = 0; i < pageSize; i++)
        {
            uint8_t expected = frames[pageNumber].getFrame()[i];
            if (data[i] != expected)
            {
                match = false;
                break;
            }
        }

        std::cout << "Page " << pageNumber << " - Read data: ";
        for (int i = 0; i < data.size(); i++)
            std::cout << formatByte(data[i]) << " ";

        if (match)
            std::cout << " - MATCH";
        else
        {
            std::cout << " - NO MATCH";
            allMatch = false;
        }
        std::cout << std::endl;
    }

    if (allMatch)
        std::cout << "All pages match what was written." << std::endl;
    else
        std::cout << "Some pages do not match." << std::endl;
}

int main()
{
    test_write();
    test_read();
    test_free();
    return 0;
}