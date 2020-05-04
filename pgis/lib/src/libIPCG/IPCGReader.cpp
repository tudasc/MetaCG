#include "IPCGReader.h"

/** RN: note that the format is child -> parent for whatever reason.. */
void IPCGAnal::build(CallgraphManager &cg, std::string filename, Config *c) {
  // CallgraphManager *cg = new CallgraphManager(c);

  std::ifstream file(filename);
  std::string line;

  std::string child;

  while (std::getline(file, line)) {
    if (line.empty()) {
      continue;
    }

    if (line.front() == '-') {
      // parent
      if (child.empty()) {
        continue;
      }
      std::string parent = line.substr(2);
      cg.putEdge(parent, std::string(), 0, child, 0, 0.0, 0, 0);
    } else {
      // child
      if (line.find("DUMMY") == 0) {
        continue;  // for some reason the graph has these dummy edges
      }

      auto endPos = line.rfind(" ");  // space between name and numStmts
      child = line.substr(0, endPos);
      int childNumStmts = 0;
      if (line.substr(line.rfind(' ') + 1) == std::string("ND")) {
        // CI: there was no defintion for the function or methods.
        //    ND = Not Definied
      } else {
        childNumStmts = std::stoi(line.substr(endPos));
        cg.putNumberOfStatements(child, childNumStmts);
      }
    }
  }

  cg.printDOT("reader");
}

int IPCGAnal::addRuntimeDispatchCallsFromCubexProfile(CallgraphManager &ipcg, CallgraphManager &cubecg) {
  // we iterate over all nodes of profile and check whether the IPCG based graph
  // has the edge attached. If not, we insert it. This is pessimistic (safe for
  // us) in the sense that, as a worst we instrument more than needed.
  int numNewlyInsertedEdges = 0;
  for (const auto cubeNode : cubecg) {
    CgNodePtr ipcgEquivNode = ipcg.findOrCreateNode(cubeNode->getFunctionName());
    // Technically it can be, but we really want to know if so!
    assert(ipcgEquivNode != nullptr && "In the profile cannot be statically unknown nodes!");
    // now we want to compare the child nodes
    for (const auto callee : cubeNode->getChildNodes()) {
      auto res = ipcgEquivNode->getChildNodes().find(callee);  // XXX How is this comparison actually done?
      if (res == ipcgEquivNode->getChildNodes().end()) {
        // we do not have the call stored!
        ipcg.putEdge(cubeNode->getFunctionName(), "ndef", 0, callee->getFunctionName(), callee->getNumberOfCalls(), .0,
                     0, 0);
        numNewlyInsertedEdges++;
      }
    }
  }
  return numNewlyInsertedEdges;
}

namespace IPCGAnal {

FuncMapT::mapped_type &getOrInsert(std::string function, FuncMapT &functions) {
  if (functions.find(function) != functions.end()) {
    auto &fi = functions[function];
    return fi;
  } else {
    FunctionInfo fi;
    fi.functionName = function;
    functions.insert({function, fi});
    auto &rfi = functions[function];
    return rfi;
  }
}

void buildFromJSON(CallgraphManager &cgm, std::string filename, Config *c) {
  using json = nlohmann::json;

  FuncMapT functions;

  std::cout << "About to read-in file" << std::endl;
  json j;
  {
    std::ifstream in(filename);
    if (!in.is_open()) {
      std::cerr << "[ERROR] opening file failed." << std::endl;
    }
    std::cout << "Reading ... " << std::endl;
    in >> j;
  }
  std::cout << "Done reading in file." << std::endl;

  for (json::iterator it = j.begin(); it != j.end(); ++it) {
    auto &fi = getOrInsert(it.key(), functions);

    fi.functionName = it.key();
    int ns = it.value()["numStatements"].get<int>();
    // ns == -1 means that there was no definition.
    if (ns > -1) {
      fi.numStatements = ns;
      fi.isVirtual = it.value()["isVirtual"].get<bool>();
      fi.doesOverride = it.value()["doesOverride"].get<bool>();
    }
    auto callees = it.value()["callees"].get<std::set<std::string>>();
    fi.callees.insert(callees.begin(), callees.end());

    auto ofs = it.value()["overriddenFunctions"].get<std::set<std::string>>();
    fi.overriddenFunctions.insert(ofs.begin(), ofs.end());

    auto ps = it.value()["parents"].get<std::set<std::string>>();
    fi.parents.insert(ps.begin(), ps.end());
  }

  // Now the functions map holds all the information
  std::unordered_map<std::string, std::unordered_set<std::string>> vCallSites;
  for (const auto pfi : functions) {
    if (pfi.second.isVirtual) {
      vCallSites.insert({pfi.first, pfi.second.parents});
      for (const auto ovcs : pfi.second.overriddenFunctions) {
        vCallSites.insert({ovcs, {pfi.first}});
      }
      // auto &vcs = vCallSites[pfi.first];
      // vcs.insert(std::begin(pfi.second.overriddenFunctions), std::end(pfi.second.overriddenFunctions));
    }
  }

  for (const auto pfi : functions) {
    for (const auto c : pfi.second.callees) {
      cgm.putEdge(pfi.first, "", 0, c, 0, .0, 0, 0);
    }
    if (pfi.second.isVirtual || pfi.second.doesOverride) {
      for (const auto om : pfi.second.overriddenFunctions) {
        for (const auto vcs : vCallSites[om]) {
          // parent -> child: vcs -> pfi.first
          cgm.putEdge(om, "", 0, pfi.first, 0, .0, 0, 0);
        }
      }
    }
    cgm.putNumberOfStatements(pfi.first, pfi.second.numStatements);
  }
}

}  // namespace IPCGAnal
