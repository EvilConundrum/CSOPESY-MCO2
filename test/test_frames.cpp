#include <iostream>
#include "../src/model/Memory.cpp"

void test_frame_read_write()
{
    Frame frame(1, 0, 16); // frame number 1, starting at logical address 0, size 16 bytes
    int addressOffset = 0;

    // Write uint16_t data
    std::vector<uint16_t> valueToWrite;

    int size = 1;
    for (int i = 1; i <= 1 << 8; i = i << 1)
    {
        if (i != 0)
            valueToWrite.push_back(i);
    }

    for (const auto &value : valueToWrite)
    {
        frame.writeData(addressOffset, value);
        addressOffset += 2;
    }

    std::cout << frame.getFrameAsString() << std::endl;
}

void test_frame_out_of_bounds()
{
    Frame frame(2, 0, 16); // frame number 2, starting at logical address 0, size 16 bytes

    // Attempt to write beyond frame size
    bool writeResult = frame.writeData(15, 0x1234); // This should fail as it exceeds frame size

    if (!writeResult)
    {
        std::cout << "Out of Bounds Write Test Passed: Write operation correctly failed." << std::endl;
    }
    else
    {
        std::cout << "Out of Bounds Write Test Failed: Write operation should have failed but succeeded." << std::endl;
    }

    // Attempt to read beyond frame size
    uint16_t readValue = frame.getData(15); // This should return 0

    if (readValue == 0)
    {
        std::cout << "Out of Bounds Read Test Passed: Read operation correctly returned 0." << std::endl;
    }
    else
    {
        std::cout << "Out of Bounds Read Test Failed: Read operation should have returned 0 but got " << readValue << std::endl;
    }
}

int main()
{
    test_frame_read_write();
    test_frame_out_of_bounds();
    return 0;
}