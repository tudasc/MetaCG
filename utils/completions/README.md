# Bash Completions

MetaCG provides a shared completion utility to add Bash autocompletion to command-line tools.

The completion script is based on the [bash-completion](https://github.com/scop/bash-completion) project.

## Adding completions to a tool

First, link the completion library to your executable in CMake:

```cmake
add_metacg_completion(cgcollector2)
```

Then include the completion header:

```cpp
#include <metacg/completion/Completion.h>
```

Check for completion requests before normal argument parsing:

```cpp
if (const auto rc = metacg::completion::handleCompletion(
        argc, argv, [&](std::string_view partial) {
            // Call the appropriate completion handler here.
        })) {
    return *rc;
}
```

### LLVM command-line options

For tools using `llvm::cl`, include `LLVMCompletion.h` and pass the option category:

```cpp
metacg::completion::completeLLVMOptions(partial, optionCategory);
```

### cxxopts

For tools using `cxxopts`, include `CXXOptsCompletion.h` and pass the `cxxopts::Options` instance:

```cpp
metacg::completion::completeCXXOptsOptions(partial, options);
```

### Manually specified options

For tools that parse command-line arguments manually, include `CompletionList.h` and provide a list of available candidates:

```cpp
const std::vector<std::string_view> commands = {
    "Why",
    "are",
    "you",
    "not",
    "using",
    "cxxopts"
};

metacg::completion::completeList(partial, commands);
```

## Enabling Bash completions

A Bash completion script is provided in this directory.

To enable completions for the current shell session, source the script:

```bash
source path/to/metacg_completion.bash
```
