/**
 * File: InternalMetadata.cpp
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "cgcollector2/metadata/Internal/ASTNodeMetadata.h"
#include "cgcollector2/metadata/Internal/AllAliasMetadata.h"
#include "cgcollector2/metadata/Internal/FunctionSignatureMetadata.h"

// Force emission of the static registrars so loading this library (e.g. via LD_PRELOAD)
// registers the metadata with MetaDataFactory.
//template struct metacg::MetaData::Registrar<ASTNodeMetadata>;
//template struct metacg::MetaData::Registrar<AllAliasMetadata>;
//template struct metacg::MetaData::Registrar<FunctionSignatureMetadata>;
