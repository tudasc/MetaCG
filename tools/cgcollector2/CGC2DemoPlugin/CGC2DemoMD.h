/**
 * File: CGC2DemoMD.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#ifndef METACG_CGCOLLECTOR2_CGC2DEMOMD_h
#define METACG_CGCOLLECTOR2_CGC2DEMOMD_h

#include "metacg/metadata/MetaData.h"

namespace clang {
    class FunctionDecl;
}


class CGC2DemoMD : public metacg::MetaData::Registrar<CGC2DemoMD> {
public:
    static constexpr const char* key = "CGC2DemoMetadata";

    CGC2DemoMD() = default;
    /**
     *  Specify how to create your Metadata from a json input
     */
    explicit CGC2DemoMD(const nlohmann::json&, metacg::StrToNodeMapping&) {
        assert(false && "This metadata should not be created via json");
    }

    explicit CGC2DemoMD(const void* const f) : address(reinterpret_cast<uintptr_t>(f)) {}
    explicit CGC2DemoMD(const uintptr_t p) : address(p) {}

    /**
     * Specify how to serialize your metadata into a json output
     * Return an empty json object to signal that this metadata will not be serialized
     * @return empty json object
     */
    nlohmann::json toJson(metacg::NodeToStrMapping&) const final {
        //We just store the address
        return {{"Address", reinterpret_cast<uintptr_t>(address)}};
    }

    /**
     * Allows to query the metadata-key from a baseclass-pointer via virtual dispatch
     * @return your metadata key
     */
    [[nodiscard]] const char* getKey() const final { return key; }

    /**
     * If your metadata stores ids of other callgraph-nodes, they might be invalidated
     * This can happen if the graph containing your metadata is merged with another graph.
     * Your metadata might be copied into the new graph
     * In this case you need to remap the stored node-ids to be valid in the new merged graph context
     *
     * @param g A map how to rename the old node-id to a new node-id, which is valid in the merged graph
     */
    void applyMapping(const metacg::GraphMapping& g) final {}

    /**
     * How to merge your metadata with itself
     * This can happen if the graph containing your metadata is merged with another graph.
     * Update the state of your this metadata object accordingly
     * @param toMerge the other Metadata to merge with
     * @param mergeAction contains whether the merge will replace the other metadata
     * @param g A map how to rename the old node-id to a new node-id, which is valid in the merged graph
     */
    void merge(const MetaData& toMerge, std::optional<metacg::MergeAction> mergeAction,
               const metacg::GraphMapping& g) final {
        if (std::strcmp(toMerge.getKey(), getKey()) != 0) {
            metacg::MCGLogger::instance().getErrConsole()->error(
                "The MetaData which was tried to merge with ASTNodeMetadata was of a different MetaData type");
            abort();
        }
        assert(false && "This metadata can not be exported and therefore is not mergeable");
        // const ASTNodeMetadata* toMergeDerived = static_cast<const ASTNodeMetadata*>(&toMerge);
    }

    /**
     * Specify how to clone your metadat
     * @return a cloned version of your metadata
     */
    [[nodiscard]] std::unique_ptr<MetaData> clone() const final {
        return std::make_unique<CGC2DemoMD>();
    }

    uintptr_t getAdressValue() {
        return reinterpret_cast<uintptr_t>(address);
    }

private:
    uintptr_t address = 0;
};
#endif
