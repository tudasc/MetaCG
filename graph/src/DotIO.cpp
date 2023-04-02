/**
 * File: DotIO.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "DotIO.h"
#include "LoggerUtil.h"
#include "MCGManager.h"
#include "Util.h"

#include <cctype>  // for std:isspace
#include <fstream>
#include <iostream>
#include <sstream>

namespace metacg::io::dot {

void Tokenizer::stripWhitespaceAndInsert(const std::string &line, std::string::size_type curPos,
                                         std::string::size_type startPos, std::vector<std::string> &tokenStrs) {
  const auto strCp = line.substr(startPos, (curPos - startPos));
  std::string strippedStr;
  std::copy_if(strCp.begin(), strCp.end(), std::back_inserter(strippedStr), [](char c) { return !std::isspace(c); });
  if (strippedStr.empty()) {
    return;
  }
  tokenStrs.emplace_back(strippedStr);
}

std::vector<std::string> Tokenizer::splitToTokenStrings(const std::string &line) {
  std::string::size_type startPos, curPos;
  std::vector<std::string> tokenStrs;
  startPos = curPos = 0;
  enum LastSeen { DIGIT, CHAR, CNTRL } ls;
  for (const auto c : line) {
    if (std::isspace(c) || c == '=' || c == '{' || c == '}' || c == '[' || c == ']' || c == '\"') {
      stripWhitespaceAndInsert(line, curPos, startPos, tokenStrs);
      startPos = curPos;
      ls = CNTRL;
    } else if (std::isdigit(c)) {
      // Digits are only parsed as number
      ls = DIGIT;
    } else if (std::isalpha(c)) {
      // Once alpha is found, rest is parsed as string
      if (ls == DIGIT || ls == CNTRL) {
        // We need to commit the digits, and start a new token.
        stripWhitespaceAndInsert(line, curPos, startPos, tokenStrs);
        startPos = curPos;
      }
      ls = CHAR;
    }
    curPos++;
  }

  return tokenStrs;
}

std::vector<ParsedToken> Tokenizer::tokenize(const std::string &line) {
  const auto strings = splitToTokenStrings(line);
  std::vector<ParsedToken> tokens;
  tokens.reserve(strings.size());
  for (const auto &str : strings) {
    if (std::isalnum(str.front())) {
      // This is an entity
      tokens.emplace_back(ParsedToken::TokenType::ENTITY, str);
    } else if (str == "->") {
      // Directed edge
      tokens.emplace_back(ParsedToken::TokenType::CONNECTOR, str);
    } else if (str == "--") {
      // undirected edge: should we support it?
    } else {
      metacg::MCGLogger::instance().getErrConsole()->error("Unexpected character found");
      tokens.emplace_back(ParsedToken::TokenType::IGNORE, str);
    }
  }
  return tokens;
}

void DotParser::parse(const std::string &line) {
  using TokenType = dot::ParsedToken::TokenType;
  dot::Tokenizer tokenizer;
  for (const auto &token : tokenizer.tokenize(line)) {
    switch (token.type) {
      case TokenType::IGNORE:
        reduceStack();
        break;
      case TokenType::ENTITY:
        handleEntity(token);
        break;
      case TokenType::CONNECTOR:
        handleConnector(token);
        break;
      default:
        metacg::MCGLogger::instance().getConsole()->error("Default case in DotParser.");
    }
  }
}

void DotParser::handleEntity(const dot::ParsedToken &token) {
  // Account for graph type
  if (state == ParseState::INIT && token.spelling == "digraph") {
    state = ParseState::NAME;
    return;
  }

  // Account for graph name
  if (state == ParseState::NAME && !token.spelling.empty()) {
    state = ParseState::GRAPH;
    return;
  }

  // Build the actual graph
  if (state == ParseState::GRAPH) {
    if (seenTokens.empty()) {
      seenTokens.push(token);
    }
    // in case the top is a connector, we connect the entities
    if (seenTokens.top().type == dot::ParsedToken::TokenType::CONNECTOR) {
      seenTokens.push(token);
      reduceStack();

      seenTokens.push(token);
    } else {
      seenTokens.pop();        // Remove currently held entity, as this is not a connector
      seenTokens.push(token);  // Push current entity as the potential source for a connector
      callgraph->getOrInsertNode(token.spelling);
    }
  } else {
    MCGLogger::instance().getErrConsole()->warn("DotParser in unclear state");
  }
}

void DotParser::handleConnector(const dot::ParsedToken &token) { seenTokens.push(token); }

void DotParser::reduceStack() {
  if (seenTokens.size() < 3) {
    MCGLogger::instance().getErrConsole()->warn("One token on token stack. Improper dot?");
    return;
  }
  while (!seenTokens.empty()) {
    auto targetElement = seenTokens.top();
    assert(targetElement.type == dot::ParsedToken::TokenType::ENTITY);
    seenTokens.pop();
    assert(seenTokens.size() > 1);
    assert(seenTokens.top().type == dot::ParsedToken::TokenType::CONNECTOR);
    seenTokens.pop();
    if (seenTokens.empty()) {
      return;  // No more entities on stack, return
    }
    auto srcElement = seenTokens.top();
    assert(srcElement.type == dot::ParsedToken::TokenType::ENTITY);
    seenTokens.pop();

    callgraph->addEdge(callgraph->getOrInsertNode(srcElement.spelling),
                       callgraph->getOrInsertNode(targetElement.spelling));
  }
}

bool DotReader::readAndManage(const std::string &cgName) {
  auto graph = std::make_unique<metacg::Callgraph>();
  auto console = metacg::MCGLogger::instance().getConsole();

  auto &sourceStream = source.getDotString();
  std::string line;

  DotParser parser(graph.get());
  while (std::getline(sourceStream, line)) {
    parser.parse(line);
  }
  console->debug("Read dot graph from {} into graph {}", source.getDescription(), cgName);
  return manager.addToManagedGraphs(cgName, std::move(graph), setActive);
}

void DotGenerator::output(DotOutputLocation outputLocation) {
  auto filename = outputLocation.path + '/' + outputLocation.fileBaseName + '-' + outputLocation.dotName + ".dot";
  metacg::MCGLogger::instance().getConsole()->info("Exporting dot to file: {}", filename);
  std::ofstream outF(filename);
  outF << dotGraphStr << std::endl;
}

namespace impl {
/** Lexicographical sorting within the edge container for easier testing */
struct CGDotEdgeComparator {
  explicit CGDotEdgeComparator(const Callgraph *cg) : cg(cg) {}

  bool operator()(const std::pair<size_t, size_t> &l, const std::pair<size_t, size_t> &r) const {
    const auto lName = cg->getNode(l.first)->getFunctionName() + cg->getNode(l.second)->getFunctionName();
    const auto rName = cg->getNode(r.first)->getFunctionName() + cg->getNode(r.second)->getFunctionName();
    return lName < rName;
  }

  const Callgraph *cg;
};

/**
 * Iterates the call graph to populate edge and node sets.
 */
template <typename EdgeContainerTy, typename NodeContainerTy>
void fillNodesAndEdges(const Callgraph *cg, NodeContainerTy &nodeNames, EdgeContainerTy &edges) {
  for (const auto &node : cg->getNodes()) {
    nodeNames.insert(node.second->getFunctionName());

    const auto childNodes = cg->getCallees(node.first);
    const auto parentNodes = cg->getCallers(node.first);

    for (const auto &c : childNodes) {
      edges.emplace(std::make_pair(node.first, c->getId()));
    }

    for (const auto &p : parentNodes) {
      edges.emplace(std::make_pair(p->getId(), node.first));
    }
  }
}

/**
 * Outputs the node set into the out stream, appending new lines.
 */
template <typename NodeContainerTy>
void getNodesStringStream(NodeContainerTy &nodes, std::stringstream &outStr) {
  for (const auto &nodeName : nodes) {
    const std::string nodeLabel{"  \"" + nodeName + '\"'};
    outStr << nodeLabel << '\n';
  }
}

/**
 * Prepares node and edge sets and outputs the list of nodes into out stream.
 */
template <typename EdgeSetTy, typename NodeSetTy>
void generateDotString(const Callgraph *cg, std::stringstream &outStr, EdgeSetTy &edges) {
  NodeSetTy nodes;

  fillNodesAndEdges<EdgeSetTy, NodeSetTy>(cg, nodes, edges);

  getNodesStringStream(nodes, outStr);

  if (!edges.empty()) {
    outStr << '\n';
  }
}

}  // namespace impl

/**
 * Generates a sorted or unsorted list of edges and an unsorted list of nodes
 * for the dot representation of the call graph in outStr.
 */
template <typename EdgeContainerTy, typename NodeContainerTy>
void generateDotString(const Callgraph *cg, EdgeContainerTy &edges, std::stringstream &outStr) {
  impl::generateDotString<EdgeContainerTy, NodeContainerTy>(cg, outStr, edges);

  // Go over edge set to output edges once
  for (const auto &p : edges) {
    const auto edgeStr =
        (cg->getNode(p.first))->getFunctionName() + " -> " + (cg->getNode(p.second))->getFunctionName();
    outStr << "  " << edgeStr << '\n';
  }
}

/**
 * Generates the actual DOT string that can be rendered by the graphviz package.
 */
void DotGenerator::generate() {
  std::stringstream dotGraphStrStr;

  dotGraphStrStr << "digraph callgraph {\n";

  if (outputSorted) {
    using EdgeSetTy = std::set<std::pair<size_t, size_t>, impl::CGDotEdgeComparator>;
    using NodeSetTy = std::set<std::string>;
    impl::CGDotEdgeComparator comparator(cg);
    EdgeSetTy edges(comparator);
    generateDotString<EdgeSetTy, NodeSetTy>(cg, edges, dotGraphStrStr);
  } else {
    using EdgeSetTy = std::unordered_set<std::pair<size_t, size_t>>;
    using NodeSetTy = std::unordered_set<std::string>;
    EdgeSetTy edges;
    generateDotString<EdgeSetTy, NodeSetTy>(cg, edges, dotGraphStrStr);
  }

  dotGraphStrStr << "}\n";
  auto logger = metacg::MCGLogger::instance().getConsole();
  logger->debug("The generated dot string:\n{}", dotGraphStrStr.str());

  dotGraphStr = dotGraphStrStr.str();
}

}  // namespace metacg::io::dot
