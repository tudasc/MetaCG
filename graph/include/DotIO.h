/**
 * File: DotIO.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#ifndef METACG_DOTIO_H
#define METACG_DOTIO_H

#include <Callgraph.h>
#include <fstream>
#include <stack>
#include <string>

namespace metacg::io {

namespace dot {
/**
 * Represents a token in the dot Tokenizer.
 * We get either ENTITY tokens (nodes), CONNECTOR tokens (edges), or IGNORE tokens (everything else).
 */
struct ParsedToken {
  enum class TokenType { IGNORE, ENTITY, CONNECTOR };
  TokenType type;
  std::string spelling;
  ParsedToken(TokenType type, const std::string& str) : type(type), spelling(str) {}
};
bool operator==(const metacg::io::dot::ParsedToken& t1, const metacg::io::dot::ParsedToken& t2) {
  return t1.type == t2.type && t1.spelling == t2.spelling;
}

/**
 * Creates a stream of ParsedToken to feed the dot Parser.
 */
class Tokenizer {
 public:
  std::vector<std::string> splitToTokenStrings(const std::string& line);
  std::vector<ParsedToken> tokenize(const std::string& line);

 private:
  void stripWhitespaceAndInsert(const std::string& line, std::string::size_type curPos, std::string::size_type startPos,
                                std::vector<std::string>& tokenStrs);
};

/**
 * Simple dot-graph parser that expects graph in simple dot files, like
 * graph callgraph {
 *   a
 *   b
 *   a -> b -> c
 * }
 */
class DotParser {
 public:
  /**
   * Construct a DotParser to construct the graph into *cg.
   */
  explicit DotParser(metacg::Callgraph* cg) : callgraph(cg) {}

  /**
   * Parses a given dot string to create the graph in to the CG passed at construction time.
   * @param line
   */
  void parse(const std::string& line);

 private:
  void handleEntity(const dot::ParsedToken& token);
  void handleConnector(const dot::ParsedToken& token);
  void reduceStack();

  enum class ParseState { INIT, NAME, GRAPH };
  ParseState state{ParseState::INIT};
  metacg::Callgraph* callgraph{nullptr};
  std::stack<dot::ParsedToken> seenTokens;
};

/**
 * Abstract base for different input types.
 * Allows to derive File- and String input classes used for easier testing.
 */
struct DotReaderSource {
  virtual std::istream& getDotString() = 0;
  virtual std::string getDescription() { return readerSrcDesc; }
  std::string readerSrcDesc;
};

/**
 * Provides a file handle to the DotReader.
 */
struct DotFileSource : public DotReaderSource {
  explicit DotFileSource(const std::string& filename) : inFile(filename) { readerSrcDesc = filename; }
  std::istream& getDotString() override { return inFile; }
  std::ifstream inFile;
};

/**
 * Provides a string to the DotReader.
 */
struct DotStringSource : public DotReaderSource {
  explicit DotStringSource(const std::string& dotString) : iss(dotString) { readerSrcDesc = "DotStringSource"; }
  std::istream& getDotString() override { return iss; }
  std::istringstream iss;
};

/**
 * Reads a simple dot graph from a DotReaderSource and constructs a MetaCG with the given name, managed by the
 * MCGManager.
 */
class DotReader {
 public:
  explicit DotReader(metacg::graph::MCGManager& manager, DotReaderSource& readerSource, bool setActive = true)
      : manager(manager), source(readerSource), setActive(setActive) {}
  bool readAndManage(const std::string& cgName);

 private:
  metacg::graph::MCGManager& manager;
  DotReaderSource& source;
  bool setActive;
};

/**
 * Denotes where to put a Dot graph. Will be used to write to
 * path/fileBaseName-dotName.dot
 */
struct DotOutputLocation {
  std::string path;
  std::string dotName;
  std::string fileBaseName;
};

/**
 * Generates a dot representation of the graph that can be persisted into a file.
 */
class DotGenerator {
 public:
  explicit DotGenerator(const metacg::Callgraph* graph, const bool sortedEdges = true)
      : cg(graph), outputSorted(sortedEdges) {}

  void output(DotOutputLocation outputLocation);

  void generate();

  [[nodiscard]] std::string getDotString() const { return dotGraphStr; }

 private:
  std::string dotGraphStr;
  const metacg::Callgraph* cg;
  bool outputSorted;
};

template <typename Decorator>
class DecoratedDotGenerator {
  // Todo: Implement a possibility to output nodes / metadata specially beautified.
};

}  // namespace dot
}  // namespace metacg::io
#endif  // METACG_DOTIO_H
