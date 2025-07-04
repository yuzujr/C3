#include <gtest/gtest.h>

#include <atomic>
#include <chrono>
#include <mutex>
#include <thread>
#include <vector>

#include "core/core.h"

// 测试专用的输出收集器
class ThreadSafeOutputCollector {
private:
    mutable std::mutex outputs_mutex;
    std::vector<nlohmann::json> outputs;
    std::atomic<size_t> output_count{0};

public:
    void collect(const nlohmann::json& output) {
        std::lock_guard<std::mutex> lock(outputs_mutex);
        outputs.push_back(output);
        output_count.store(outputs.size());

        // 调试输出 - 区分不同类型的输出
        if (output.contains("data") && output["data"].contains("output")) {
            std::string output_text =
                output["data"]["output"].get<std::string>();
            std::string session_id =
                output.contains("session_id")
                    ? output["session_id"].get<std::string>()
                    : "unknown";

            if (!output_text.empty()) {
                std::cout << "[COLLECTED] Session: " << session_id
                          << ", Output: '" << output_text << "'" << std::endl;
            } else {
                std::cout << "[COLLECTED] Session: " << session_id
                          << ", Empty output (write result)" << std::endl;
            }
        }
    }

    void clear() {
        std::lock_guard<std::mutex> lock(outputs_mutex);
        outputs.clear();
        output_count.store(0);
    }

    size_t count() const {
        return output_count.load();
    }

    std::vector<nlohmann::json> get_outputs() const {
        std::lock_guard<std::mutex> lock(outputs_mutex);
        return outputs;
    }

    bool has_output_containing(const std::string& text) const {
        std::lock_guard<std::mutex> lock(outputs_mutex);
        for (const auto& output : outputs) {
            if (output.contains("data") && output["data"].contains("output")) {
                std::string output_text =
                    output["data"]["output"].get<std::string>();
                // 只检查非空输出，忽略写入操作的空响应
                if (!output_text.empty() &&
                    output_text.find(text) != std::string::npos) {
                    return true;
                }
            }
        }
        return false;
    }

    // 等待特定的输出出现
    bool wait_for_output_containing(const std::string& text,
                                    std::chrono::milliseconds timeout =
                                        std::chrono::milliseconds(5000)) const {
        auto start = std::chrono::steady_clock::now();
        while (std::chrono::steady_clock::now() - start < timeout) {
            if (has_output_containing(text)) {
                return true;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
        return false;
    }

    // 调试：打印所有收集到的输出
    void debug_print_all() const {
        std::lock_guard<std::mutex> lock(outputs_mutex);
        std::cout << "=== All Collected Outputs (" << outputs.size()
                  << ") ===" << std::endl;
        for (size_t i = 0; i < outputs.size(); ++i) {
            std::cout << "[" << i << "] " << outputs[i].dump() << std::endl;
        }
        std::cout << "=== End Debug Output ===" << std::endl;
    }
};

// 全局输出收集器
static ThreadSafeOutputCollector g_collector;

// 测试夹具
class PtyManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 重置PTY管理器状态
        PtyManager::getInstance().reset();

        // 清理收集器
        g_collector.clear();

        // 设置输出回调
        PtyManager::getInstance().setOutputCallback(
            [](const nlohmann::json& response) {
                g_collector.collect(response);
            });
    }

    void TearDown() override {
        // 关闭所有会话
        PtyManager::getInstance().shutdownAllPtySessions();

        // 清理收集器
        g_collector.clear();
    }
};

// 基础PTY会话测试
TEST_F(PtyManagerTest, BasicPtySession) {
    // 写入简单命令
    PtyManager::getInstance().writeToPtySession("test_basic",
                                                "echo Hello PTY\r\n");

    // 给命令更多时间执行
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 等待输出
    EXPECT_TRUE(g_collector.wait_for_output_containing("Hello PTY"))
        << "Should receive echo output within timeout";

    EXPECT_GT(g_collector.count(), 0) << "Should have received some output";

    // 如果测试失败，打印调试信息
    if (!g_collector.has_output_containing("Hello PTY")) {
        g_collector.debug_print_all();
    }

    // 关闭会话
    auto result = PtyManager::getInstance().closePtySession("test_basic");
    EXPECT_TRUE(result["success"].get<bool>())
        << "Should successfully close session";
}

// 多会话测试
TEST_F(PtyManagerTest, MultipleSessions) {
    // 创建多个会话
    PtyManager::getInstance().writeToPtySession("session1",
                                                "echo Session-1-Output\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    PtyManager::getInstance().writeToPtySession("session2",
                                                "echo Session-2-Output\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 等待两个会话的输出
    EXPECT_TRUE(g_collector.wait_for_output_containing("Session-1-Output"))
        << "Should receive session 1 output";
    EXPECT_TRUE(g_collector.wait_for_output_containing("Session-2-Output"))
        << "Should receive session 2 output";

    // 如果测试失败，打印调试信息
    if (!g_collector.has_output_containing("Session-1-Output") ||
        !g_collector.has_output_containing("Session-2-Output")) {
        g_collector.debug_print_all();
    }

    // 关闭所有会话
    auto result1 = PtyManager::getInstance().closePtySession("session1");
    auto result2 = PtyManager::getInstance().closePtySession("session2");

    EXPECT_TRUE(result1["success"].get<bool>()) << "Should close session1";
    EXPECT_TRUE(result2["success"].get<bool>()) << "Should close session2";
}

// PTY调整大小测试
TEST_F(PtyManagerTest, PtyResize) {
    // 创建会话
    PtyManager::getInstance().writeToPtySession("resize_test",
                                                "echo Resize Test\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 测试调整大小
    auto resize_result =
        PtyManager::getInstance().resizePtySession("resize_test", 120, 40);
    EXPECT_TRUE(resize_result["success"].get<bool>())
        << "Should successfully resize PTY";

    // 清理
    PtyManager::getInstance().closePtySession("resize_test");
}

// 不存在会话操作测试
TEST_F(PtyManagerTest, NonexistentSessionOperations) {
    // 测试对不存在会话的操作
    auto resize_result =
        PtyManager::getInstance().resizePtySession("nonexistent", 80, 24);
    EXPECT_FALSE(resize_result["success"].get<bool>())
        << "Should fail to resize nonexistent session";

    auto close_result =
        PtyManager::getInstance().closePtySession("nonexistent");
    EXPECT_FALSE(close_result["success"].get<bool>())
        << "Should fail to close nonexistent session";
}

// 关闭所有会话测试
TEST_F(PtyManagerTest, ShutdownAllSessions) {
    // 创建多个会话
    PtyManager::getInstance().writeToPtySession("shutdown_test1",
                                                "echo Test1\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    PtyManager::getInstance().writeToPtySession("shutdown_test2",
                                                "echo Test2\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(200));

    PtyManager::getInstance().writeToPtySession("shutdown_test3",
                                                "echo Test3\r\n");
    std::this_thread::sleep_for(std::chrono::milliseconds(500));

    // 关闭所有会话
    PtyManager::getInstance().shutdownAllPtySessions();

    // 等待一下确保会话被关闭
    std::this_thread::sleep_for(std::chrono::milliseconds(100));

    // 验证会话已关闭（尝试操作应该失败）
    auto resize_result =
        PtyManager::getInstance().resizePtySession("shutdown_test1", 80, 24);
    EXPECT_FALSE(resize_result["success"].get<bool>())
        << "Session should be closed after shutdown";
}

// 性能测试：快速创建和关闭会话
TEST_F(PtyManagerTest, PerformanceTest) {
    const int num_sessions = 10;
    auto start = std::chrono::high_resolution_clock::now();

    // 创建多个会话
    for (int i = 0; i < num_sessions; ++i) {
        std::string session_id = "perf_test_" + std::to_string(i);
        PtyManager::getInstance().writeToPtySession(
            session_id, "echo Performance Test " + std::to_string(i) + "\r\n");
    }

    // 等待所有输出
    std::this_thread::sleep_for(std::chrono::seconds(2));

    // 关闭所有会话
    for (int i = 0; i < num_sessions; ++i) {
        std::string session_id = "perf_test_" + std::to_string(i);
        PtyManager::getInstance().closePtySession(session_id);
    }

    auto end = std::chrono::high_resolution_clock::now();
    auto duration =
        std::chrono::duration_cast<std::chrono::milliseconds>(end - start);

    std::cout << "Performance test completed in " << duration.count() << "ms"
              << std::endl;
    EXPECT_LT(duration.count(), 10000) << "Should complete within 10 seconds";
}

// 参数化测试：测试不同的命令
class PtyCommandTest : public PtyManagerTest,
                       public ::testing::WithParamInterface<
                           std::pair<std::string, std::string>> {};

TEST_P(PtyCommandTest, DifferentCommands) {
    auto [command, expected_output] = GetParam();

    PtyManager::getInstance().writeToPtySession("cmd_test", command + "\r\n");

    // 给命令时间执行
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));

    // 等待输出
    EXPECT_TRUE(g_collector.wait_for_output_containing(expected_output))
        << "Command '" << command << "' should produce output containing '"
        << expected_output << "'";

    // 如果测试失败，打印调试信息
    if (!g_collector.has_output_containing(expected_output)) {
        std::cout << "=== Failed Command Test Debug ===" << std::endl;
        std::cout << "Command: " << command << std::endl;
        std::cout << "Expected: " << expected_output << std::endl;
        g_collector.debug_print_all();
    }

    PtyManager::getInstance().closePtySession("cmd_test");
}

INSTANTIATE_TEST_SUITE_P(
    BasicCommands, PtyCommandTest,
    ::testing::Values(std::make_pair("echo Hello", "Hello"),
                      std::make_pair("echo World", "World"),
                      std::make_pair("echo 123", "123")));

int main(int argc, char** argv) {
    // 初始化日志（只初始化一次）
    Logger::init(spdlog::level::info, spdlog::level::debug);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
