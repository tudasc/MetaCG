#pragma once

#include "Diff.h"
#include <string>

struct GlobalMDDiff : Diff {

    GlobalMDDiff(std::string key_,
                 std::string aValue_,
                 std::string bValue_)
        : key(std::move(key_)),
        aValue(aValue_),
        bValue(bValue_) {}

    std::string key;
    std::string aValue;
    std::string bValue; 
    Kind kind() const override { return Kind::GlobalMD; }
    nlohmann::ordered_json toJson() const override {
        nlohmann::ordered_json inner;
        inner["aValue"] = aValue;
        inner["bValue"] = bValue;
        return inner;
    }
};
