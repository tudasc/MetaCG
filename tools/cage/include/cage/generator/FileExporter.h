/**
 * File: FileExporter.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_FILEEXPORTER_H
#define METACG_FILEEXPORTER_H

#include "cage/generator/CallGraphConsumer.h"

namespace cage {

class FileExporter : public CallGraphConsumer {
  void consumeCallGraph(metacg::Callgraph&) override;
};

}  // namespace cage

#endif  // METACG_FILEEXPORTER_H
