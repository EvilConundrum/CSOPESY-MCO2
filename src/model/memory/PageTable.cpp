typedef struct Frame
{

} Frame;

typedef struct PageTableEntry
{
    unsigned int pageNumber;  // virtual page number
    unsigned int frameNumber; // pointer to frame in physical memory
    bool valid;
    bool dirty;
} PageTableEntry;

PageTableEntry createPageTableEntry(unsigned int pageNumber,
                                    unsigned int frameNumber)
{
    PageTableEntry entry;
    entry.pageNumber = pageNumber;
    entry.frameNumber = frameNumber;
    entry.valid = false;
    entry.dirty = false;
    return entry;
}
