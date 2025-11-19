#include <iostream>
#include <fstream>
#include <cstdio>
#include <cassert>
#include <memory>
#include <string>
#include "../src/handlers/ScreenHandler.cpp"

#define TEST(name) void name()
#define ASSERT_TRUE(x) assert(x)
#define ASSERT_FALSE(x) assert(!(x))
#define ASSERT_EQ(x, y) assert((x) == (y))
#define ASSERT_NE(x, y) assert((x) != (y))
#define RUN_TEST(test) { std::cout << "  " << #test << "... "; test(); std::cout << "PASSED\n"; }

struct ScreenTestContext {
    std::unique_ptr<Config> config;
    std::unique_ptr<GeneratorHandler> generator;
    std::unique_ptr<ScreenHandler> handler;

    ScreenTestContext()
    {
        config = std::make_unique<Config>("config.txt");
        generator = std::make_unique<GeneratorHandler>();
        handler = std::make_unique<ScreenHandler>(config.get(), generator.get(), nullptr, nullptr);
    }
};

TEST(test_screen_create_registers_process)
{
    ScreenTestContext ctx;
    ASSERT_TRUE(ctx.handler->createScreen("screen-test-1"));
    ASSERT_TRUE(ctx.handler->processExists("screen-test-1"));
    ASSERT_EQ(ctx.handler->getProcessCount(), 1);
}

TEST(test_screen_summary_contains_process)
{
    ScreenTestContext ctx;
    ctx.handler->createScreen("screen-alpha");

    std::string summary = ctx.handler->generateScreenReport(false);
    ASSERT_NE(summary.find("Running processes"), std::string::npos);
    ASSERT_NE(summary.find("screen-alpha"), std::string::npos);
}

TEST(test_screen_report_file_contains_process)
{
    ScreenTestContext ctx;
    ctx.handler->createScreen("screen-beta");

    const std::string filename = "test_screen_report_tmp.txt";
    ASSERT_TRUE(ctx.handler->writeScreenReport(filename));

    std::ifstream file(filename);
    ASSERT_TRUE(file.is_open());
    std::string contents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    file.close();
    ASSERT_NE(contents.find("screen-beta"), std::string::npos);

    std::remove(filename.c_str());
}

int main()
{
    std::cout << "Running ScreenHandler tests...\n";
    RUN_TEST(test_screen_create_registers_process);
    RUN_TEST(test_screen_summary_contains_process);
    RUN_TEST(test_screen_report_file_contains_process);
    std::cout << "All ScreenHandler tests passed.\n";
    return 0;
}
