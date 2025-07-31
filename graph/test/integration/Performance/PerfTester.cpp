/**
 * File: PerfTester.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "Callgraph.h"
#include "io/MCGReader.h"
#include "io/MCGWriter.h"

#include <chrono>
#include <functional>
#include <random>

using namespace metacg;

class Timer {
 public:
  using Clock = std::chrono::high_resolution_clock;
  using TP = Clock::time_point;
  using Duration = Clock::duration;

  explicit Timer() {start();};

  void start() { startTime = Clock::now();}

  void stop() { endTime = Clock::now(); }

  std::chrono::milliseconds getElapsedMillis() {
    auto dur = endTime - startTime;
    auto durInMillis = std::chrono::duration_cast<std::chrono::milliseconds>(dur);
    return durInMillis;
  }

 private:
  TP startTime;
  TP endTime;
};

using PerfResults = std::unordered_map<std::string, size_t>;

class PerfTester {
 public:
  explicit PerfTester(int seed, int numNodes, int numEdges) : seed(seed), numNodes(numNodes), numEdges(numEdges), rng(seed) {}

  PerfResults run();

 private:
  template<typename Fn, typename... ArgT>
  void runPerfTest(PerfResults&, const std::string&, Fn&& testFn, ArgT&&... args);

  void insertNodes(Callgraph& cg, int num);
  void insertEdges(Callgraph& cg, int num);
  void write(Callgraph& cg, io::JsonSink& sink, int version);
  void read(std::unique_ptr<Callgraph>& cg, io::JsonSource& source);

 private:
  int seed;
  int numNodes;
  int numEdges;
  std::mt19937 rng;
};

PerfResults PerfTester::run() {
  using namespace std::placeholders;

  PerfResults results;

  auto cg = std::make_unique<Callgraph>();
  auto insertTest = std::bind(&PerfTester::insertNodes, *this, _1, _2);
  auto insertEdges = std::bind(&PerfTester::insertEdges, *this, _1, _2);
  runPerfTest(results, "insert nodes", insertTest, *cg, numNodes);
  runPerfTest(results, "insert edges", insertEdges, *cg, numEdges);

  auto writeTest = std::bind(&PerfTester::write, *this, _1, _2, _3);
  io::JsonSink v4Sink;
  runPerfTest(results, "write V4 format", writeTest, *cg, v4Sink, 4);

  auto readTest = std::bind(&PerfTester::read, *this, _1, _2);
  std::unique_ptr<Callgraph> readCg;
  auto v4Source = io::JsonSource(v4Sink.getJson());
  runPerfTest(results, "read V4 format", readTest, readCg, v4Source);

  io::JsonSink v2Sink;
  runPerfTest(results, "write V2 format", writeTest, *cg, v2Sink, 2);

  std::unique_ptr<Callgraph> readCg2;
  auto v2Source = io::JsonSource(v2Sink.getJson());
  runPerfTest(results, "read V2 format", readTest, readCg, v2Source);

  return results;
}

template<typename Fn, typename... ArgT>
void PerfTester::runPerfTest(PerfResults& results, const std::string& name, Fn&& testFn, ArgT&&... args) {
  std::cout  << "Running performance test: " << name << "\n";
  Timer timer;
  testFn(std::forward<ArgT>(args)...);
  timer.stop();
  results[name] = timer.getElapsedMillis().count();
}

void PerfTester::insertNodes(Callgraph& cg, int num) {
  for (int i = 0; i < num; i++) {
    cg.insert("function" + std::to_string(i));
  }
  assert(cg.size() == num && "Size of CG does not match inserted nodes");
}

void PerfTester::insertEdges(Callgraph& cg, int num) {
  std::uniform_int_distribution<> dist(0, cg.size()-1);
  auto& nodes = cg.getNodes();
  for (int i = 0; i < num; i++) {
    auto name1 = "function" + std::to_string(dist(rng));
    auto name2 = "function" + std::to_string(dist(rng));
    auto node1 = cg.getFirstNode(name1);
    auto node2 = cg.getFirstNode(name2);
    assert(node1 && node2 && "Node does not exist");
    cg.addEdge(*node1, *node2);
  }
}

void PerfTester::write(Callgraph& cg, io::JsonSink& sink, int version) {
  auto writer = io::createWriter(version);
  assert(writer && "Writer is null");
  writer->write(&cg, sink);
}

void PerfTester::read(std::unique_ptr<Callgraph>& cg, io::JsonSource& source) {
  auto reader = io::createReader(source);
  cg = std::move(reader->read());
}



int main(int argc, const char** argv) {
  int numNodes = 10000;
  int numEdges = 20000;
  int seed = 463248923;
  if (argc > 1) {
    numNodes = std::stoi(argv[1]);
    if (argc > 2) {
      numEdges = std::stoi(argv[2]);
      if (argc > 3) {
        seed = std::stoi(argv[3]);
      }
    } else {
      numEdges = numNodes;
    }
  }

  PerfTester perfTest(seed, numNodes, numEdges);
  auto results = perfTest.run();

  // Determine column widths
  size_t nameWidth = 0;
  for (const auto& [test, _] : results) {
    nameWidth = std::max(nameWidth, test.size());
  }
  nameWidth = std::max(nameWidth, std::string("Test Name").size());

  std::cout << "Test results for " << numNodes << " nodes and " << numEdges << " edges (seed: " << seed << ")\n";
  // Print header
  std::cout << std::string(nameWidth + 2 + 10, '-') << "\n";
  std::cout << std::left << std::setw(nameWidth + 2) << "Test Name"
            << std::right << std::setw(10) << "Time (ms)" << "\n";
  std::cout << std::string(nameWidth + 2 + 10, '-') << "\n";

  // Print rows
  for (const auto& [test, millis] : results) {
    std::cout << std::left << std::setw(nameWidth + 2) << test
              << std::right << std::setw(10) << millis << "\n";
  }
  std::cout << std::string(nameWidth + 2 + 10, '-') << "\n";



  return 0;
}