# How to contribute

Thanks for considering to contribute to metacg.
We are currently developing in a non-public repository.
In case you want to contribute, please reach out through the issues and we see how we progress.

## Issues

We are aware that MetaCG has open issues, i.e., a correct handling of function pointers, etc.
If you find a new issue that is not already reported in github, please feel free to open an issue.
The issue should state clearly what is the problem, and what you did to get there.

If you can, please provide a minimal working example in the issue, and the tool output.
Please also share, what you expected the tool to do.

## Workflow

We currently accept changes via merge requests in our non-public repository.
All history will be squashed in the merge.

**Internal**: Please always branch from `devel`, and only in very particular bugfix cases (really critical ones) ask in our slack what to do.
Preceed a branch either with `fix/` or with `feat/` to indicate what it is meant to do, followed by the issue number.

## Tests

Every contribution must add reasonable tests.

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

