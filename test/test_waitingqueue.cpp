#include <iostream>
#include <cassert>
#include <memory>
#include "../src/model/WaitingQueue.cpp"

#define TEST(name) void name()
#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))
#define ASSERT_EQ(x, y) assert((x) == (y))
#define RUN_TEST(test) { std::cout << "  " << #test << "... "; test(); std::cout << "PASSED\n"; }

// Test: Basic waiting queue operations
TEST(test_waiting_queue_basic) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    
    waitQueue.enqueueProcess(p1, 3);
    ASSERT_FALSE(waitQueue.empty());
    ASSERT_EQ(waitQueue.size(), 1);
    ASSERT_EQ(p1->getState(), ProcessState::WAITING);
    
    auto dequeued = waitQueue.dequeueProcess();
    ASSERT_EQ(dequeued->getPID(), "P1");
    ASSERT_TRUE(waitQueue.empty());
}

// Test: Waiting queue peek
TEST(test_waiting_queue_peek) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    
    waitQueue.enqueueProcess(p1, 5);
    waitQueue.enqueueProcess(p2, 5);
    
    auto peeked = waitQueue.peek();
    ASSERT_EQ(peeked->getPID(), "P1");
    ASSERT_EQ(waitQueue.size(), 2); // Unchanged
}

// Test: Waiting queue getAllProcesses
TEST(test_waiting_queue_get_all) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    auto p3 = std::make_shared<Process>("P3", 5);
    
    waitQueue.enqueueProcess(p1, 5);
    waitQueue.enqueueProcess(p2, 5);
    waitQueue.enqueueProcess(p3, 5);
    
    auto allProcs = waitQueue.getAllProcesses();
    ASSERT_EQ(allProcs.size(), 3);
    ASSERT_EQ(allProcs[0]->getPID(), "P1");
    ASSERT_EQ(allProcs[1]->getPID(), "P2");
    ASSERT_EQ(allProcs[2]->getPID(), "P3");
    
    // Verify queue unchanged
    ASSERT_EQ(waitQueue.size(), 3);
}

// Test: Waiting queue contains
TEST(test_waiting_queue_contains) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    auto p3 = std::make_shared<Process>("P3", 5);
    
    waitQueue.enqueueProcess(p1, 2);
    waitQueue.enqueueProcess(p2, 3);
    
    ASSERT_TRUE(waitQueue.contains(p1));
    ASSERT_TRUE(waitQueue.contains(p2));
    ASSERT_FALSE(waitQueue.contains(p3));
    ASSERT_FALSE(waitQueue.contains(nullptr));
}

// Test: Contains by PID
TEST(test_waiting_queue_contains_by_pid) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p1_dup = std::make_shared<Process>("P1", 5);
    
    waitQueue.enqueueProcess(p1, 4);
    
    ASSERT_TRUE(waitQueue.contains(p1_dup));
}

// Test: Clear waiting queue
TEST(test_waiting_queue_clear) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    
    waitQueue.enqueueProcess(p1, 1);
    waitQueue.enqueueProcess(p2, 2);
    ASSERT_EQ(waitQueue.size(), 2);
    
    waitQueue.clear();
    ASSERT_TRUE(waitQueue.empty());
    ASSERT_EQ(waitQueue.size(), 0);
}

// Test: Empty waiting queue operations
TEST(test_empty_waiting_queue) {
    WaitingQueue waitQueue;
    
    ASSERT_TRUE(waitQueue.empty());
    ASSERT_EQ(waitQueue.size(), 0);
    ASSERT_EQ(waitQueue.dequeueProcess(), nullptr);
    ASSERT_EQ(waitQueue.peek(), nullptr);
    
    auto allProcs = waitQueue.getAllProcesses();
    ASSERT_EQ(allProcs.size(), 0);
}

// Test: FIFO ordering in waiting queue
TEST(test_waiting_queue_fifo_ordering) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    auto p3 = std::make_shared<Process>("P3", 5);
    
    waitQueue.enqueueProcess(p1, 2);
    waitQueue.enqueueProcess(p2, 2);
    waitQueue.enqueueProcess(p3, 2);
    
    ASSERT_EQ(waitQueue.dequeueProcess()->getPID(), "P1");
    ASSERT_EQ(waitQueue.dequeueProcess()->getPID(), "P2");
    ASSERT_EQ(waitQueue.dequeueProcess()->getPID(), "P3");
}

// Test: State changes on enqueue/dequeue
TEST(test_waiting_queue_state_management) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    
    p1->setState(ProcessState::READY);
    ASSERT_EQ(p1->getState(), ProcessState::READY);
    
    waitQueue.enqueueProcess(p1, 2);
    ASSERT_EQ(p1->getState(), ProcessState::WAITING);
}

// Test: Processes are released after the expected number of ticks
TEST(test_waiting_queue_release_after_ticks) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);

    waitQueue.enqueueProcess(p1, 2);
    waitQueue.enqueueProcess(p2, 4);

    auto ready = waitQueue.advanceTicks();
    ASSERT_EQ(ready.size(), 0);
    ASSERT_EQ(waitQueue.size(), 2);

    ready = waitQueue.advanceTicks();
    ASSERT_EQ(ready.size(), 1);
    ASSERT_EQ(ready[0]->getPID(), "P1");
    ASSERT_EQ(waitQueue.size(), 1);

    ready = waitQueue.advanceTicks(2);
    ASSERT_EQ(ready.size(), 1);
    ASSERT_EQ(ready[0]->getPID(), "P2");
    ASSERT_TRUE(waitQueue.empty());
}

// Test: Multiple enqueue/dequeue cycles
TEST(test_waiting_queue_multiple_cycles) {
    WaitingQueue waitQueue;
    
    // Add 10 processes
    for (int i = 0; i < 10; i++) {
        auto p = std::make_shared<Process>("P" + std::to_string(i), 5);
        waitQueue.enqueueProcess(p, 2);
    }
    
    ASSERT_EQ(waitQueue.size(), 10);
    
    // Remove 5
    for (int i = 0; i < 5; i++) {
        waitQueue.dequeueProcess();
    }
    
    ASSERT_EQ(waitQueue.size(), 5);
    
    // Add 5 more
    for (int i = 10; i < 15; i++) {
        auto p = std::make_shared<Process>("P" + std::to_string(i), 5);
        waitQueue.enqueueProcess(p, 2);
    }
    
    ASSERT_EQ(waitQueue.size(), 10);
}

// Test: Large waiting queue
TEST(test_large_waiting_queue) {
    WaitingQueue waitQueue;
    
    // Add 100 processes
    for (int i = 0; i < 100; i++) {
        auto p = std::make_shared<Process>("P" + std::to_string(i), 5);
        waitQueue.enqueueProcess(p, 3);
    }
    
    ASSERT_EQ(waitQueue.size(), 100);
    
    auto allProcs = waitQueue.getAllProcesses();
    ASSERT_EQ(allProcs.size(), 100);
}

// Test: Destructor clears queue
TEST(test_waiting_queue_destructor) {
    {
        WaitingQueue waitQueue;
        auto p1 = std::make_shared<Process>("P1", 5);
        auto p2 = std::make_shared<Process>("P2", 5);
        
        waitQueue.enqueueProcess(p1, 5);
        waitQueue.enqueueProcess(p2, 5);
        
        ASSERT_EQ(waitQueue.size(), 2);
        // Goes out of scope
    }
    ASSERT_TRUE(true); // No crash = pass
}

// Test: Interleaved operations
TEST(test_waiting_queue_interleaved_ops) {
    WaitingQueue waitQueue;
    auto p1 = std::make_shared<Process>("P1", 5);
    auto p2 = std::make_shared<Process>("P2", 5);
    auto p3 = std::make_shared<Process>("P3", 5);
    
    waitQueue.enqueueProcess(p1, 3);
    ASSERT_EQ(waitQueue.size(), 1);
    
    auto peeked = waitQueue.peek();
    ASSERT_EQ(peeked->getPID(), "P1");
    
    waitQueue.enqueueProcess(p2, 4);
    ASSERT_EQ(waitQueue.size(), 2);
    
    auto dequeued = waitQueue.dequeueProcess();
    ASSERT_EQ(dequeued->getPID(), "P1");
    ASSERT_EQ(waitQueue.size(), 1);
    
    waitQueue.enqueueProcess(p3, 6);
    ASSERT_EQ(waitQueue.size(), 2);
    
    ASSERT_TRUE(waitQueue.contains(p2));
    ASSERT_TRUE(waitQueue.contains(p3));
    ASSERT_FALSE(waitQueue.contains(p1));
}

int main() {
    std::cout << "=== Running WaitingQueue Unit Tests ===\n";
    
    try {
        RUN_TEST(test_waiting_queue_basic);
        RUN_TEST(test_waiting_queue_peek);
        RUN_TEST(test_waiting_queue_get_all);
        RUN_TEST(test_waiting_queue_contains);
        RUN_TEST(test_waiting_queue_contains_by_pid);
        RUN_TEST(test_waiting_queue_clear);
        RUN_TEST(test_empty_waiting_queue);
        RUN_TEST(test_waiting_queue_fifo_ordering);
        RUN_TEST(test_waiting_queue_state_management);
        RUN_TEST(test_waiting_queue_release_after_ticks);
        RUN_TEST(test_waiting_queue_multiple_cycles);
        RUN_TEST(test_large_waiting_queue);
        RUN_TEST(test_waiting_queue_destructor);
        RUN_TEST(test_waiting_queue_interleaved_ops);
        
        std::cout << "\n=== All WaitingQueue tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n=== Test failed: " << e.what() << " ===\n";
        return 1;
    }
}
