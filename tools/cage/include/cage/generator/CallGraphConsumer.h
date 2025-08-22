/**
 * File: CallGraphConsumer.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_CALLGRAPHCONSUMER_H
#define METACG_CALLGRAPHCONSUMER_H

namespace metacg{
class Callgraph;
}

namespace cage {

struct CallGraphConsumer {
  virtual ~CallGraphConsumer() = default;
  virtual void consumeCallGraph(metacg::Callgraph&) = 0;
};

}  // namespace cage

#endif  // METACG_CALLGRAPHCONSUMER_H
