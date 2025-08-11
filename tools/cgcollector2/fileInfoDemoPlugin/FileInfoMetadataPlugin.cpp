/**
* File: FileInfoMetadataPlugin.cpp
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/

#include <clang/AST/ASTContext.h>
#include <clang/Basic/SourceManager.h>

#include "FileInfoMetadata.h"
#include "Plugin.h"

/**
 * Plugin that only generates declaration based metadata
 * The generated metadata is metacg compatible and defined above
 */

struct FileInfoMetadataPlugin : Plugin {
  virtual std::unique_ptr<metacg::MetaData> computeForDecl(const clang::FunctionDecl* const functionDecl) {
    std::unique_ptr<FileInfoMetadata> result = std::make_unique<FileInfoMetadata>();
    const auto sourceLocation = functionDecl->getLocation();
    auto& astCtx = functionDecl->getASTContext();
    const auto fullSrcLoc = astCtx.getFullLoc(sourceLocation);
    const auto fileEntry = fullSrcLoc.getFileEntry();
    if (!fileEntry) {
      return result;
    }

    const auto fileName = fileEntry->getName();
    std::string fileNameStr = fileName.str();
    result->fromSystemInclude = astCtx.getSourceManager().isInSystemHeader(sourceLocation);
    result->origin = fileNameStr;
    return result;
  };

  FileInfoMetadataPlugin() = default;
};

// This is the function which is used to get the FileInformationMetadataPlugin struct
extern "C" {
FileInfoMetadataPlugin* getPlugin() { return new FileInfoMetadataPlugin(); }
}