#pragma once
#include "Command.h"

struct ReachesCommand : Command {
    int run(const std::vector<char*>& args) override;
    std::string name() const override { return "reaches"; }
};
