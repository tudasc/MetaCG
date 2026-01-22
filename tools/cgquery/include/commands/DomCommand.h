/**
* File: DomCommand.h
* License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
* https://github.com/tudasc/metacg/LICENSE.txt
*/
#pragma once
#include "Command.h"

struct DomCommand : Command {
    int run(const std::vector<char*>& args) override;
    std::string name() const override { return "dom"; };
};
