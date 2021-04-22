# How to contribute

Thanks for considering to contribute to MetaCG.
We are currently developing in a non-public repository.
In case you want to contribute, please reach out through the issues and we see how we progress.

## Issues

We are aware that MetaCG has open issues, i.e., a correct handling of function pointers, etc.
If you find a new issue that is not already reported in github, please feel free to open an issue.
The issue should state clearly what is the problem, and what you did to get there.

If you can, please provide a MWE in the issue, and the tool output.
Please also share, what you have the tool expected to do.

## Workflow

Please contribute only via pull requests and (a) keep your commit history clean, or (b) it will be squashed during merge.

**Internal**: Please branch from `master` for fixes and from `devel` for researchy new stuff.
`master` is automatically mirrored to github, so, please, only merge to `master`, if the CI builds succeed.
Preceed a branch either with `fix/` or with `feat/` to indicate it.

## Tests

Every contribution must add reasonable tests.
(We use a Gitlab internally for development, so the github CI / actions are not yet set up.)

## Commit messages

Please write commit messages in an active voice and have a brief summary in the first line, one empty line and a little more thorough explanation what is added, if reasonable.
For example, the commit message may read as

~~~{.txt}
Adds a cool functionality and removes some obsolete code

- Adds feature X, which does this and that
- Removes the old and never used code path Y
~~~

## Code style

Please use clang-format to format the sources before opening the pull request.
Please follow the naming schemes and capitalization.

