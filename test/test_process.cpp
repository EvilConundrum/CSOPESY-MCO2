#include <iostream>
#include <cassert>
#include <memory>
#include <thread>
#include <chrono>
#include "../src/model/Process.cpp"

#define TEST(name) void name()
#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))
#define ASSERT_EQ(x, y) assert((x) == (y))
#define ASSERT_NE(x, y) assert((x) != (y))
#define RUN_TEST(test) { std::cout << "  " << #test << "... "; test(); std::cout << "PASSED\n"; }

// Test: Process creation with default values
TEST(test_process_creation) {
    Process p("P1", 10);
    ASSERT_EQ(p.getPID(), "P1");
    ASSERT_EQ(p.getTotalInstructions(), 10);
    ASSERT_EQ(p.getCurrentLine(), 0);
    ASSERT_EQ(p.getState(), ProcessState::READY);
    ASSERT_FALSE(p.isFinished());
}

// Test: Process state transitions
TEST(test_process_state_transitions) {
    Process p("P1", 5);
    
    ASSERT_EQ(p.getState(), ProcessState::READY);
    
    p.setState(ProcessState::RUNNING);
    ASSERT_EQ(p.getState(), ProcessState::RUNNING);
    
    p.setState(ProcessState::WAITING);
    ASSERT_EQ(p.getState(), ProcessState::WAITING);
    
    p.setState(ProcessState::FINISHED);
    ASSERT_EQ(p.getState(), ProcessState::FINISHED);
    ASSERT_TRUE(p.isFinished());
}

// Test: Adding instructions
TEST(test_add_instructions) {
    Process p("P1", 0);
    
    Instruction inst1("PRINT", {"Hello"});
    Instruction inst2("DECLARE", {"x", "5"});
    
    p.addInstruction(inst1);
    p.addInstruction(inst2);
    
    ASSERT_EQ(p.getTotalInstructions(), 2);
}

// Test: Execute simple instructions
TEST(test_execute_simple_instructions) {
    Process p("P1", 2);
    
    Instruction printInst("PRINT", {"Hello"});
    Instruction declareInst("DECLARE", {"x", "10"});
    
    p.addInstruction(printInst);
    p.addInstruction(declareInst);
    
    ASSERT_TRUE(p.hasInstructions());
    ASSERT_EQ(p.getCurrentLine(), 0);
    
    bool executed = p.executeNextInstruction();
    ASSERT_TRUE(executed);
    ASSERT_EQ(p.getCurrentLine(), 1);
    
    executed = p.executeNextInstruction();
    ASSERT_TRUE(executed);
    ASSERT_EQ(p.getCurrentLine(), 2);
    
    ASSERT_FALSE(p.hasInstructions());
}

// Test: Process finishes after all instructions
TEST(test_process_finishes_after_instructions) {
    Process p("P1", 2);
    
    p.addInstruction(Instruction("PRINT", {"Test"}));
    p.addInstruction(Instruction("DECLARE", {"x", "5"}));
    
    ASSERT_FALSE(p.isFinished());
    
    p.executeNextInstruction();
    ASSERT_FALSE(p.isFinished());
    
    p.executeNextInstruction();
    ASSERT_TRUE(p.isFinished());
}

// Test: Reset process
TEST(test_process_reset) {
    Process p("P1", 3);
    
    p.addInstruction(Instruction("DECLARE", {"x", "10"}));
    p.addInstruction(Instruction("ADD", {"x", "5"}));
    p.addInstruction(Instruction("PRINT", {"x"}));
    
    // Execute all instructions
    p.executeNextInstruction();
    p.executeNextInstruction();
    p.executeNextInstruction();
    
    ASSERT_EQ(p.getCurrentLine(), 3);
    ASSERT_TRUE(p.isFinished());
    
    // Reset
    p.reset();
    
    ASSERT_EQ(p.getCurrentLine(), 0);
    ASSERT_EQ(p.getState(), ProcessState::READY);
    ASSERT_FALSE(p.isFinished());
    ASSERT_TRUE(p.hasInstructions());
}

// Test: Process creation time
TEST(test_process_creation_time) {
    auto beforeCreation = std::chrono::system_clock::now();
    Process p("P1", 5);
    auto afterCreation = std::chrono::system_clock::now();
    
    auto creationTime = p.getCreationTime();
    
    ASSERT_TRUE(creationTime >= beforeCreation);
    ASSERT_TRUE(creationTime <= afterCreation);
    
    std::string timeStr = p.getCreationTimeStr();
    ASSERT_FALSE(timeStr.empty());
}

// Test: Get current instruction
TEST(test_get_current_instruction) {
    Process p("P1", 2);
    
    Instruction inst1("PRINT", {"Hello"});
    Instruction inst2("DECLARE", {"x", "10"});
    
    p.addInstruction(inst1);
    p.addInstruction(inst2);
    
    auto current = p.getCurrentInstruction();
    ASSERT_EQ(current.getCommand(), "PRINT");
    
    p.executeNextInstruction();
    
    current = p.getCurrentInstruction();
    ASSERT_EQ(current.getCommand(), "DECLARE");
}

// Test: Process with no instructions
TEST(test_process_no_instructions) {
    Process p("P1", 0);
    
    ASSERT_EQ(p.getTotalInstructions(), 0);
    ASSERT_FALSE(p.hasInstructions());
    ASSERT_TRUE(p.isFinished());
}

// Test: Process execution with variable storage
TEST(test_process_variable_storage) {
    Process p("P1", 3);
    
    p.addInstruction(Instruction("DECLARE", {"x", "10"}));
    p.addInstruction(Instruction("ADD", {"x", "x", "5"}));
    p.addInstruction(Instruction("SUBTRACT", {"x", "x", "3"}));
    
    // Execute instructions
    p.executeNextInstruction(); // x = 10
    p.executeNextInstruction(); // x = 15
    p.executeNextInstruction(); // x = 12
    
    ASSERT_TRUE(p.isFinished());
}

// Test: Multiple processes with same PID
TEST(test_multiple_processes_different_pids) {
    Process p1("P1", 5);
    Process p2("P2", 5);
    Process p3("P1", 5); // Same PID as p1
    
    ASSERT_EQ(p1.getPID(), "P1");
    ASSERT_EQ(p2.getPID(), "P2");
    ASSERT_EQ(p3.getPID(), "P1");
    ASSERT_NE(p1.getPID(), p2.getPID());
}

// Test: Process state after manual finish
TEST(test_manual_finish_state) {
    Process p("P1", 10);
    
    ASSERT_FALSE(p.isFinished());
    
    p.setState(ProcessState::FINISHED);
    
    ASSERT_TRUE(p.isFinished());
    ASSERT_EQ(p.getState(), ProcessState::FINISHED);
}

// Test: Large number of instructions
TEST(test_large_instruction_count) {
    Process p("P1", 1000);
    
    for (int i = 0; i < 1000; i++) {
        p.addInstruction(Instruction("PRINT", {"Test"}));
    }
    
    ASSERT_EQ(p.getTotalInstructions(), 1000);
    
    // Execute first 100
    for (int i = 0; i < 100; i++) {
        ASSERT_TRUE(p.hasInstructions());
        p.executeNextInstruction();
    }
    
    ASSERT_EQ(p.getCurrentLine(), 100);
    ASSERT_FALSE(p.isFinished());
}

int main() {
    std::cout << "=== Running Process Unit Tests ===\n";
    
    try {
        RUN_TEST(test_process_creation);
        RUN_TEST(test_process_state_transitions);
        RUN_TEST(test_add_instructions);
        RUN_TEST(test_execute_simple_instructions);
        RUN_TEST(test_process_finishes_after_instructions);
        RUN_TEST(test_process_reset);
        RUN_TEST(test_process_creation_time);
        RUN_TEST(test_get_current_instruction);
        RUN_TEST(test_process_no_instructions);
        RUN_TEST(test_process_variable_storage);
        RUN_TEST(test_multiple_processes_different_pids);
        RUN_TEST(test_manual_finish_state);
        RUN_TEST(test_large_instruction_count);
        
        std::cout << "\n=== All Process tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n=== Test failed: " << e.what() << " ===\n";
        return 1;
    }
}
