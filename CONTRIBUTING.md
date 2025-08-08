# How to contribute

Thanks for considering to contribute to MetaCG.
We are migrating to GitHub for development and follow the typical GitHub workflow: fork the repository, work in a feature branch, open a PR against the MetaCG repo.

## Issues

We are aware that MetaCG has open issues, i.e., a correct handling of function pointers, etc.
If you find a new issue that is not already reported in GitHub, please feel free to open an issue.
The issue should state clearly what is the problem, and what you did to get there.

If you can, please provide a minimal working example in the issue that reproduces the problem, and the tool output.
Please also share, what you expected the tool to do.

## Workflow to Contribute

1. Fork MetaCG
2. Create a feature branch, e.g., `feat/logging-rainbows-and-unicorns`, to work on a feature, or `fix/nullpointer-deref-crash`, to work on a fix.
3. Open a PR against the MetaCG repo that contains your changes. It is typically better to have one PR per feature or fix and not combine multiple things into one PR.
4. Code review until checks pass and the PR is approved. It is left to the author to move their patch *unless* they have no write access. In this case, they should let the reviewer know and have the PR merged on their behalf
5. Squash and merge the contribution. Remember to check the title and the message of the squashed commit and adjust if necessary.

## Tests

Every contribution is expected to add reasonable tests.

## Commit Messages

Please write commit messages in an active voice and have a brief summary in the first line, one empty line and a little more thorough explanation what is added, if reasonable.
For example, the commit message may read as

~~~{.txt}
[Graph] Adds parallel reachability analysis

<< Some details here >>
~~~

## Code style

Please use git-clang-format before opening a PR.
Please follow the naming schemes and capitalization.

## MetaCG file format version

As described in the [graphlib's README](graph/README.md), we distinguish between multiple version of the JSON-based file format for serialized call graphs.
The file format version numbers are not to be confused with the MetaCG version numbers.
We consider a file format version to be final as soon as it has been merged into the `master` branch.
Before being merged to `master`, a file format version may be subject to breaking changes.
As soon as a file format version is on `master` (and thereby is potentially part of a release), breaking changes to the file format must be accompanied by an incremented version number.
