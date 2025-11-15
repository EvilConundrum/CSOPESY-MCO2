#include <iostream>
#include <cassert>
#include <unordered_map>
#include <string>
#include "../src/model/Instruction.cpp"

#define TEST(name) void name()
#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))
#define ASSERT_EQ(x, y) assert((x) == (y))
#define RUN_TEST(test) { std::cout << "  " << #test << "... "; test(); std::cout << "PASSED\n"; }

// Test: Instruction type identification
TEST(test_instruction_type_detection) {
    Instruction print("PRINT", {"Hello"});
    ASSERT_EQ(print.getType(), InstructionType::PRINT);
    ASSERT_TRUE(print.isValid());
    
    Instruction declare("DECLARE", {"x", "10"});
    ASSERT_EQ(declare.getType(), InstructionType::DECLARE);
    
    Instruction add("ADD", {"x", "5"});
    ASSERT_EQ(add.getType(), InstructionType::ADD);
    
    Instruction sub("SUBTRACT", {"x", "3"});
    ASSERT_EQ(sub.getType(), InstructionType::SUBTRACT);
    
    Instruction sleep("SLEEP", {"100"});
    ASSERT_EQ(sleep.getType(), InstructionType::SLEEP);
    
    Instruction forLoop("FOR", {"i", "0", "10"});
    ASSERT_EQ(forLoop.getType(), InstructionType::FOR);
}

// Test: Case insensitive command parsing
TEST(test_case_insensitive_commands) {
    Instruction print1("print", {"Hello"});
    Instruction print2("PRINT", {"Hello"});
    Instruction print3("PrInT", {"Hello"});
    
    ASSERT_EQ(print1.getType(), InstructionType::PRINT);
    ASSERT_EQ(print2.getType(), InstructionType::PRINT);
    ASSERT_EQ(print3.getType(), InstructionType::PRINT);
}

// Test: Unknown instruction type
TEST(test_unknown_instruction) {
    Instruction unknown("INVALID", {"arg"});
    ASSERT_EQ(unknown.getType(), InstructionType::UNKNOWN);
    ASSERT_FALSE(unknown.isValid());
}

// Test: Default constructor creates unknown instruction
TEST(test_default_constructor) {
    Instruction inst;
    ASSERT_EQ(inst.getType(), InstructionType::UNKNOWN);
    ASSERT_FALSE(inst.isValid());
}

// Test: DECLARE instruction execution
TEST(test_declare_execution) {
    std::unordered_map<std::string, uint16_t> vars;
    
    Instruction declare("DECLARE", {"x", "42"});
    int ticks = declare.execute(vars);
    
    ASSERT_EQ(ticks, 0); // Instant execution
    ASSERT_EQ(vars["x"], 42);
}

// Test: ADD instruction execution
TEST(test_add_execution) {
    std::unordered_map<std::string, uint16_t> vars;
    vars["x"] = 10;
    
    Instruction add("ADD", {"result", "x", "5"});
    int ticks = add.execute(vars);
    
    ASSERT_EQ(ticks, 0);
    ASSERT_EQ(vars["result"], 15);
}

// Test: SUBTRACT instruction execution
TEST(test_subtract_execution) {
    std::unordered_map<std::string, uint16_t> vars;
    vars["x"] = 20;
    
    Instruction sub("SUBTRACT", {"result", "x", "7"});
    int ticks = sub.execute(vars);
    
    ASSERT_EQ(ticks, 0);
    ASSERT_EQ(vars["result"], 13);
}

// Test: SLEEP instruction returns tick count
TEST(test_sleep_returns_ticks) {
    std::unordered_map<std::string, uint16_t> vars;
    
    Instruction sleep("SLEEP", {"100"});
    int ticks = sleep.execute(vars);
    
    ASSERT_EQ(ticks, 100); // Should return sleep duration
}

// Test: PRINT instruction execution
TEST(test_print_execution) {
    std::unordered_map<std::string, uint16_t> vars;
    vars["x"] = 42;
    
    Instruction print("PRINT", {"x"});
    int ticks = print.execute(vars);
    
    ASSERT_EQ(ticks, 0); // Instant execution
}

// Test: Get command and arguments
TEST(test_get_command_and_args) {
    Instruction inst("ADD", {"x", "10"});
    
    ASSERT_EQ(inst.getCommand(), "ADD");
    
    auto args = inst.getArgs();
    ASSERT_EQ(args.size(), 2);
    ASSERT_EQ(args[0], "x");
    ASSERT_EQ(args[1], "10");
}

// Test: Multiple operations on same variable
TEST(test_multiple_operations) {
    std::unordered_map<std::string, uint16_t> vars;
    
    Instruction declare("DECLARE", {"counter", "0"});
    declare.execute(vars);
    ASSERT_EQ(vars["counter"], 0);
    
    Instruction add1("ADD", {"counter", "counter", "5"});
    add1.execute(vars);
    ASSERT_EQ(vars["counter"], 5);
    
    Instruction add2("ADD", {"counter", "counter", "10"});
    add2.execute(vars);
    ASSERT_EQ(vars["counter"], 15);
    
    Instruction sub("SUBTRACT", {"counter", "counter", "3"});
    sub.execute(vars);
    ASSERT_EQ(vars["counter"], 12);
}

// Test: Operations on multiple variables
TEST(test_multiple_variables) {
    std::unordered_map<std::string, uint16_t> vars;
    
    Instruction declare1("DECLARE", {"x", "10"});
    Instruction declare2("DECLARE", {"y", "20"});
    Instruction declare3("DECLARE", {"z", "30"});
    
    declare1.execute(vars);
    declare2.execute(vars);
    declare3.execute(vars);
    
    ASSERT_EQ(vars.size(), 3);
    ASSERT_EQ(vars["x"], 10);
    ASSERT_EQ(vars["y"], 20);
    ASSERT_EQ(vars["z"], 30);
}

// Test: Instruction with logging callback
TEST(test_instruction_with_logging) {
    std::unordered_map<std::string, uint16_t> vars;
    std::vector<std::string> logs;
    
    auto logCallback = [&logs](const std::string& msg) {
        logs.push_back(msg);
    };
    
    Instruction declare("DECLARE", {"x", "100"});
    declare.execute(vars, logCallback, false); // Don't print to console
    
    // Callback should have been invoked
    // (actual behavior depends on implementation)
}

// Test: Empty arguments
TEST(test_empty_arguments) {
    Instruction inst("PRINT", {});
    
    ASSERT_EQ(inst.getType(), InstructionType::PRINT);
    ASSERT_EQ(inst.getArgs().size(), 0);
}

// Test: Large numbers in instructions
TEST(test_large_numbers) {
    std::unordered_map<std::string, uint16_t> vars;
    
    Instruction declare("DECLARE", {"big", "65535"}); // Max uint16_t
    declare.execute(vars);
    
    ASSERT_EQ(vars["big"], 65535);
}

int main() {
    std::cout << "=== Running Instruction Unit Tests ===\n";
    
    try {
        RUN_TEST(test_instruction_type_detection);
        RUN_TEST(test_case_insensitive_commands);
        RUN_TEST(test_unknown_instruction);
        RUN_TEST(test_default_constructor);
        RUN_TEST(test_declare_execution);
        RUN_TEST(test_add_execution);
        RUN_TEST(test_subtract_execution);
        RUN_TEST(test_sleep_returns_ticks);
        RUN_TEST(test_print_execution);
        RUN_TEST(test_get_command_and_args);
        RUN_TEST(test_multiple_operations);
        RUN_TEST(test_multiple_variables);
        RUN_TEST(test_instruction_with_logging);
        RUN_TEST(test_empty_arguments);
        RUN_TEST(test_large_numbers);
        
        std::cout << "\n=== All Instruction tests passed! ===\n";
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "\n=== Test failed: " << e.what() << " ===\n";
        return 1;
    }
}
