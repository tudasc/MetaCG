//
// Created by jp on 17.06.22.
//

#include "DotIO.h"
#include "LoggerUtil.h"
#include "MCGManager.h"
#include "Util.h"

#include <cctype>  // for std:isspace
#include <fstream>
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
    callgraph->addEdge(srcElement.spelling, targetElement.spelling);
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

void DotGenerator::generate() {
  std::stringstream dotGraphStrStr;
  std::set<std::pair<CgNode *, CgNode *>> edges;

  dotGraphStrStr << "digraph callgraph {\n";
  // First build all node labels and a set of all edges
  for (const auto &node : *cg) {
    const auto nodeName = node->getFunctionName();
    const auto childNodes = node->getChildNodes();
    const auto parentNodes = node->getParentNodes();
    const std::string nodeLabel{"  \"" + nodeName + '\"'};
    dotGraphStrStr << nodeLabel << '\n';

    for (const auto &c : childNodes) {
      edges.insert({node.get(), c.get()});
    }
    for (const auto &p : parentNodes) {
      edges.insert({p.get(), node.get()});
    }
  }

  // Plainly for "better" reading / formatting
  if (!edges.empty()) {
    dotGraphStrStr << '\n';
  }
  // Go over edge set to output edges once
  for (const auto &p : edges) {
    const auto edgeStr = (p.first)->getFunctionName() + " -> " + (p.second)->getFunctionName();
    dotGraphStrStr << "  " << edgeStr << '\n';
  }

  dotGraphStrStr << "}\n";
  auto logger = metacg::MCGLogger::instance().getConsole();
  logger->debug("The generated dot string:\n{}", dotGraphStrStr.str());

  dotGraphStr = dotGraphStrStr.str();
}

}  // namespace metacg::io::dot