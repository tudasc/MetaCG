#pragma once
#include "Command.h"

struct DomCommand : Command {
    int run(const std::vector<char*>& args) override;
    std::string name() const override { return "dom"; };
};
