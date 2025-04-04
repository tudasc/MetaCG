/**
 * File: LoggingTest.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "LoggerUtil.h"
#include "gtest/gtest.h"

class LoggingTest : public ::testing::Test {
 protected:
  void SetUp() override {
    //Need to set log level to trace, to test all levels
    //Default log level does not include trace and debug outputs
    spdlog::set_level(spdlog::level::trace);
  }
};

static void removeDate(std::string& log) { log.erase(0, log.find_first_of(']') + 2); }

TEST_F(LoggingTest, InfoConsoleDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.info("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [info] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, InfoConsoleUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.info<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.info<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [info] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, InfoErrorDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.info<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::DEFAULT>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [info] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, InfoErrorUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.info<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.info<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [info] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, DebugConsoleDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.debug("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [debug] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, DebugConsoleUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.debug<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.debug<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [debug] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, DebugErrorDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.debug<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::DEFAULT>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [debug] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, DebugErrorUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.debug<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.debug<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [debug] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, WarnConsoleDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.warn("Test String {}, {} ", 1, 2.3f);
  // logger.getConsole()->warn("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [warning] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, WarnConsoleUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.warn<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.warn<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [warning] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, WarnErrorDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.warn<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::DEFAULT>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [warning] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, WarnErrorUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.warn<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.warn<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [warning] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, ErrorConsoleDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.error("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [error] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, ErrorConsoleUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.error<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.error<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [error] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, ErrorErrorDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.error<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::DEFAULT>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [error] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, ErrorErrorUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.error<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.error<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [error] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, TraceConsoleDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.trace("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [trace] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, TraceConsoleUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.trace<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.trace<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [trace] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, TraceErrorDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.trace<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::DEFAULT>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [trace] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, TraceErrorUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.trace<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.trace<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [trace] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, CriticalConsoleDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.critical("Test String {}, {} ", 1, 2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [critical] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, CriticalConsoleUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStdout();
  logger.critical<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1,
                                                                                          2.3f);
  logger.critical<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1,
                                                                                          2.3f);
  std::string output = testing::internal::GetCapturedStdout();
  removeDate(output);
  ASSERT_EQ(output, "[console] [critical] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, CriticalErrorDefault) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.critical<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::DEFAULT>("Test String {}, {} ", 1,
                                                                                         2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [critical] Test String 1, 2.3 \n");
}

TEST_F(LoggingTest, CriticalErrorUnique) {
  auto logger = metacg::MCGLogger::instance();
  testing::internal::CaptureStderr();
  logger.critical<metacg::MCGLogger::Output::ERROR, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1, 2.3f);
  logger.critical<metacg::MCGLogger::Output::DEFAULT, metacg::MCGLogger::LogType::UNIQUE>("Test String {}, {} ", 1,
                                                                                          2.3f);
  std::string output = testing::internal::GetCapturedStderr();
  removeDate(output);
  ASSERT_EQ(output, "[errconsole] [critical] Test String 1, 2.3 \n");
}