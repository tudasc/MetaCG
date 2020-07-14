#include "nlohmann/json.hpp"

#include <fstream>

#include <iostream>

int main(int argc, char **argv) {
  if (argc < 3) {
    return -1;
  }

  std::vector<std::string> inputFiles(argc - 2);
  for (int i = 2; i < argc; ++i) {
    inputFiles[i - 2] = argv[i];
  }

  nlohmann::json wholeCG;

  for (auto &filename : inputFiles) {
    std::ifstream file(filename);
    nlohmann::json current;
    file >> current;

    for (nlohmann::json::iterator it = current.begin(); it != current.end(); ++it) {
      // j[getMangledName(f_decl)] = {
      //   {"callees", callees},
      //   {"isVirtual", isVirtual},
      //   {"doesOverride", doesOverride},
      //   {"overriddenFunctions", overriddenFunctions},
      //   {"overriddenBy", overriddenBy},
      //   {"parents", callers}
      // };

      if (wholeCG[it.key()].empty()) {
        wholeCG[it.key()] = it.value();
      } else {
        auto &c = wholeCG[it.key()];
        auto &v = it.value();
        // TODO multiple bodies possible, if the body is in header?
        if (v["hasBody"].get<bool>() && c["hasBody"].get<bool>()) {
          std::cout << "WARNING: merge of " << it.key() << " has detected multiple bodies" << std::endl;
          // TODO check for equal values
        } else if (v["hasBody"].get<bool>()) {
          c["callees"] = v["callees"];
          c["isVirtual"] = v["isVirtual"];
          c["doesOverride"] = v["doesOverride"];
          c["overriddenFunctions"] = v["overriddenFunctions"];

          c["hasBody"] = v["hasBody"];

          // TODO separate merge of meta information
          c["numStatements"] = v["numStatements"];
        } else if (c["hasBody"].get<bool>()) {
          // callees, isVirtual, doesOverride and overriddenFunctions unchanged
          // hasBody unchanged
          // numStatements unchanged
        } else {
          // nothing special
        }

        // merge overriden by and caller
        {
          auto &c_over = c["overriddenBy"];
          auto &v_over = v["overriddenBy"];

          for (auto &s : v_over) {
            c_over.push_back(s.get<std::string>());
          }

          auto &c_parents = c["parents"];
          auto &v_parents = v["parents"];

          for (auto &s : v_parents) {
            c_parents.push_back(s.get<std::string>());
          }
        }
      }
    }
  }

  std::ofstream file(argv[1]);
  file << wholeCG;

  return 0;
}
