#include "metacg/io/VersionFourMCGReader.h"
#include "metacg/io/VersionFourMCGWriter.h"
#include "metacg/Callgraph.h"
#include "metacg/MergePolicy.h"
#include <iostream>

// Forward declaration of the existing task function from Callgraph.cpp
extern void task(metacg::Callgraph* cg);

int main(int argc, char** argv) {
    if (argc != 3) {
        std::cerr << "Usage: " << argv[0] << " cg_a.gtmcg cg_b.gtmcg" << std::endl;
        return -1;
    }

    metacg::io::FileSource fsA(argv[1]);
    metacg::io::FileSource fsB(argv[2]);
    metacg::io::VersionFourMCGReader readerA(fsA);
    metacg::io::VersionFourMCGReader readerB(fsB);

    auto cgA = readerA.read();
    auto cgB = readerB.read();

    // Call merge with task as postprocessing functor
    cgA->merge(*cgB, metacg::MergeByName{}, task);

    metacg::io::VersionFourMCGWriter writer;
    writer.setExportSorted(true);
    metacg::io::JsonSink js;
    writer.write(cgA.get(), js);
    std::cout << js.getJson().dump(4) << std::endl;

    return 0;
}