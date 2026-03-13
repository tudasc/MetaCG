/**
* File: FileInfoCollector.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#ifndef CGCOLLECTOR2_FILEINFOCOLLECTOR_H
#define CGCOLLECTOR2_FILEINFOCOLLECTOR_H
#include "cgcollector2/interface/Plugin.h"
#include "metadata/FileInfoMD.h"

#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceManager.h>



/**
 * Plugin that only generates declaration based metadata
 * The generated metadata is metacg compatible and defined above
 */

struct FileInfoCollector : cgcollector2::Plugin {

  virtual std::unique_ptr<metacg::MetaData> computeForDecl(const clang::FunctionDecl* const functionDecl) {
    std::unique_ptr<FileInfoMetadata> result = std::make_unique<FileInfoMetadata>();
    const auto sourceLocation = functionDecl->getLocation();
    auto& astCtx = functionDecl->getASTContext();
    const auto fullSrcLoc = astCtx.getFullLoc(sourceLocation);

#if LLVM_VERSION_MAJOR < 18
    const auto fileEntry = fullSrcLoc.getFileEntry();
#else
    const auto fileEntry = fullSrcLoc.getFileEntryRef();
#endif
    
    if (!fileEntry) {
      return result;
    }

    const auto fileName = fileEntry->getName();
    std::string fileNameStr = fileName.str();
    result->fromSystemInclude = astCtx.getSourceManager().isInSystemHeader(sourceLocation);
    result->origin = fileNameStr;
    return result;
  };

  FileInfoCollector() = default;
};


#endif  // CGCOLLECTOR2_FILEINFOCOLLECTOR_H