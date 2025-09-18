/**
 * File: Diff.h
 * License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
 * https://github.com/tudasc/metacg/LICENSE.txt
 */
#pragma once
#include "nlohmann/json.hpp"

struct Diff {
    enum class Kind { Node, GlobalMD };
    virtual ~Diff() = default;
    virtual Kind kind() const = 0;
    virtual nlohmann::ordered_json toJson() const = 0;
};
