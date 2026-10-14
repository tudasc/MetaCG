#!/usr/bin/env python3
#
# File: TargetCollector.py
# This utility script allows to generate the whole-program call-graph for a given cmake target
# Script Version: 0.6.0
# License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
# https://github.com/tudasc/metacg/LICENSE.txt
#

import argparse
import pathlib
import shlex
import sys
import subprocess
import os
import json
from typing import Any, TypeAlias
Json: TypeAlias = dict[str, Any]
from functools import partial
from multiprocessing import Pool




# A wrapper function to allow thread pooling
# as cgcollector does not terminate on every input file,
# we allow for timeouts to happen, and inform the user,
# which cgcollector invocation timed out after how many seconds
def collector(command: tuple[int, list[str]], outStream: int | None, errStream: int | None) -> None:
    timeout, arguments = command
    try:
        subprocess.run(arguments, timeout=timeout, stdout=outStream, stderr=errStream)
    except subprocess.TimeoutExpired:
        print(f"cgcollector timed out after {timeout} seconds: {shlex.join(arguments)}")

# checks whether the user-given cmake arguments already name a source directory,
# either via -S or as a positional path, skipping the values of options that take a separate argument
def specifiesSourceDirectory(cmakeArguments: list[str]) -> bool:
    optionsWithValue: set[str] = {"-S", "-B", "-G", "-T", "-A", "-C", "-D", "-U"}
    skipNext: bool = False
    for argument in cmakeArguments:
        if skipNext:
            skipNext = False
        elif argument.startswith("-S") or not argument.startswith("-"):
            return True
        elif argument in optionsWithValue:
            skipNext = True
    return False

def generateAPI(buildDirectory: str, cmakeArgs:str, outStream: int | None, errStream: int | None) -> None:
    # cmake's API communicates over files and directories,
    # create the request
    os.makedirs(buildDirectory + "/.cmake/api/v1/query/", exist_ok=True)
    open(buildDirectory + "/.cmake/api/v1/query/codemodel-v2", "a").close()
    # run cmake to generate the answers
    # we might need to pass additional parameters to cmake to match the original compile
    # if no source directory is given, we re-run cmake on the already configured build directory
    userCmakeArguments: list[str] = shlex.split(cmakeArgs)
    cmake_command: list[str] = ["cmake", "-DCMAKE_EXPORT_COMPILE_COMMANDS=ON"] + userCmakeArguments
    if not specifiesSourceDirectory(userCmakeArguments):
        cmake_command.append(".")
    print("cd " + buildDirectory + " && " + shlex.join(cmake_command))
    subprocess.run(cmake_command, cwd=buildDirectory,stdout=outStream,stderr=errStream)

def loadTargetDescription(cmakeJsonTarget: Json, buildDirectory:str) -> Json:
    with open(buildDirectory + "/.cmake/api/v1/reply/" + cmakeJsonTarget["jsonFile"], 'r') as targetDescription:
        return json.load(targetDescription)

def getSources(cmakeJsonTarget: Json, sourceRoot:str, buildDirectory:str) -> tuple[list[str], list[str]]:
    jsonTargetDescription: Json = loadTargetDescription(cmakeJsonTarget,buildDirectory)

    # get the include flags of all compile groups, keeping their order and whether they are system includes,
    # so that warnings from e.g. LLVM headers stay suppressed (interface libraries have no compile groups)
    includeFlags: list[str] = list(dict.fromkeys(
        ("-isystem" if include.get("isSystem", False) else "-I") + include["path"]
        for compileGroup in jsonTargetDescription.get("compileGroups", [])
        for include in compileGroup.get("includes", [])))

    # only sources that are actually compiled are of interest, headers are included anyway
    # source paths are relative to the top level source directory, unless they lie outside of it
    sourcePaths: list[str] = [str(pathlib.Path(sourceRoot) / source["path"])
                              for source in jsonTargetDescription["sources"] if "compileGroupIndex" in source]
    return includeFlags, sourcePaths

def getDependencyTargets(cmakeJsonTarget: Json, allCmakeTargets: list[Json], buildDirectory:str ) -> list[Json]:
    jsonTargetDescription: Json = loadTargetDescription(cmakeJsonTarget,buildDirectory)
    if "dependencies" not in jsonTargetDescription:
        return []
    dependencyIds = [dependency["id"] for dependency in jsonTargetDescription["dependencies"]]
    return [target for target in allCmakeTargets if target["id"] in dependencyIds]


def getAllTargets(buildDirectory:str) -> tuple[list[Json],str]:

    # Find the most up-to-date reply
    reply_dir: pathlib.Path = pathlib.Path(buildDirectory + "/.cmake/api/v1/reply")
    index_path: pathlib.Path = max(reply_dir.glob("index-*.json"))

    with index_path.open() as f:
        index = json.load(f)

    # Find the codemodel-v2 file
    codemodel_ref = index["reply"]["codemodel-v2"]
    codemodel_path = index_path.parent / codemodel_ref["jsonFile"]

    with codemodel_path.open() as f:
        codemodel = json.load(f)

    allCmakeTargets: list[Json] = codemodel["configurations"][0]["targets"]
    sourceRoot: str = codemodel["paths"]["source"]
    return allCmakeTargets, sourceRoot


def parseArgs():
    # Setup command line parsing
    # use -h to get pretty list
    parser: argparse.ArgumentParser = argparse.ArgumentParser(
        description='Generate Target specific callgraphs via cmake-file api')
    parser.add_argument('-g', '--generate',
                        help='Choose whether to generate the api files, the graph, or both',
                        required=False,
                        choices=["api", "graph", "both"], default="both")
    parser.add_argument('-b', '--build-directory', type=str, metavar="<path>", default=".",
                        help="The path to the cmake-build root folder", required=False)
    parser.add_argument('-c', '--cgcollector', metavar='<path/bin>', type=str, help='the path to the cgcollector tool',
                        default="cgcollector",
                        required=False)
    parser.add_argument('-j', '--jobs', metavar='<int>',
                        help='number of parallel cgcollectors allowed to run',
                        required=False, type=int, default=1)
    parser.add_argument('-w', '--wallclock-timeout', metavar="<int>", type=int,
                        help="The time in seconds cg collector is allowed to run before being terminated", default=120)
    parser.add_argument('-m', '--cgmerge', metavar='<path/bin>', type=str, default="cgmerge",
                        help='the path to the cgmerge tool', required=False)
    parser.add_argument('-e', '--extra-args', metavar="<list>", default="", help='pipe args to clang extra-arg',
                        type=str,
                        required=False)
    parser.add_argument('-a', '--cmake-args', metavar="<list>", help='pipe args to cmake', default="", type=str,
                        required=False)
    parser.add_argument('-o', '--output', metavar="<str>", default="wholeProgramCG.ipcg", type=str,
                        help='The name of the output file',
                        required=False)
    parser.add_argument('-t', '--target', metavar="<str>", type=str,
                        help='The target name for which to generate the callgraph',
                        required=False)
    parser.add_argument('-r', '--recursive', help='Also collect dependency targets', required=False,
                        action="store_true")
    parser.add_argument('--ci-concurrent-suffix', metavar="<str>", type=str,
                        help='Suffix appended to output filename in concurrent CI runs', required=False, default="")
    parser.add_argument('-l', '--list-targets', help='List all available targets to extract', required=False ,action="store_true")
    parser.add_argument('-v', '--verbose', help='Print subprocess err-outputs', required=False ,action="store_true")
    parser.add_argument('-vv', '--very-verbose', help='Print subprocess std-outputs', required=False ,action="store_true")

    # parse all arguments after the first one, as this is the name we got called by
    return parser.parse_args(sys.argv[1:])

def main():
    parserObject = parseArgs()


    # subprocess output is discarded, unless the user asks for it (None inherits our own streams)
    outStream: int | None = None if parserObject.very_verbose else subprocess.DEVNULL
    errStream: int | None = None if parserObject.verbose or parserObject.very_verbose else subprocess.DEVNULL

    if parserObject.list_targets:
        generateAPI(parserObject.build_directory,parserObject.cmake_args,outStream,errStream)
        print([t["name"] for t in getAllTargets(parserObject.build_directory)[0]])
        exit()

    # if we want to generate a new api query file (or do both)
    if parserObject.generate in ['api', 'both']:
        generateAPI(parserObject.build_directory,parserObject.cmake_args,outStream,errStream)
    # if we want to generate a graph for the target
    if parserObject.generate in ['graph', 'both']:
        [allCmakeTargets, sourceRoot] = getAllTargets(parserObject.build_directory)

        # we need to find the answer file for our target
        # for this, we list all targets, and find ours, via its name
        targetFileList: list[Json] = [t for t in allCmakeTargets if t["name"] == parserObject.target]
        if len(targetFileList) == 0:
            print("Could not find target in cmake targets list")
            exit(1)

        target: Json = targetFileList[0]
        collectedTargets: list[Json] = [target]

        if parserObject.recursive:
            # every target is only collected once, even if multiple targets depend on it
            visitedIds: set[str] = {target["id"]}
            worklist: list[Json] = getDependencyTargets(target,allCmakeTargets,parserObject.build_directory)
            while worklist:
                dependency: Json = worklist.pop(0)
                if dependency["id"] in visitedIds:
                    continue
                visitedIds.add(dependency["id"])
                collectedTargets.append(dependency)
                worklist += getDependencyTargets(dependency,allCmakeTargets,parserObject.build_directory)

        # if a compile commands database exists, it provides the exact flags of the original compile
        # otherwise, we need to pass the include-directories to our tools
        useCompileDatabase: bool = os.path.isfile(os.path.join(parserObject.build_directory, "compile_commands.json"))
        if not useCompileDatabase:
            print("No compile_commands.json found in build directory, passing include directories instead")

        # if the user wants to pass any other arguments, we pipe them along
        userArguments: list[str] = [f'--extra-arg={i}' for i in shlex.split(parserObject.extra_args)]

        commands: list[tuple[int, list[str]]] = []
        tempIPCGs: list[str] = []
        for collectedTarget in collectedTargets:
            includeFlags, sourceFiles = getSources(collectedTarget,sourceRoot,parserObject.build_directory)
            if useCompileDatabase:
                toolArguments = ["-p", parserObject.build_directory]
            else:
                toolArguments = [f'--extra-arg={flag}' for flag in includeFlags]

            # generate a separate cgcollector command for each source
            for sourceFile in sourceFiles:
                ipcg: str = sourceFile + parserObject.ci_concurrent_suffix + '.ipcg'
                if ipcg in tempIPCGs:
                    continue
                tempIPCGs.append(ipcg)
                commands.append((parserObject.wallclock_timeout,
                                 [parserObject.cgcollector] + toolArguments + userArguments + ["--cg-file", ipcg, sourceFile]))

        # use a thread pool to run all commands in parallel (w.r.t pool-size)
        print(f"Now collecting cg from {len(commands)} files")
        with Pool(parserObject.jobs) as p:
            for finished, _ in enumerate(p.imap_unordered(partial(collector, outStream=outStream, errStream=errStream), commands), start=1):
                print(f"\r{finished}/{len(commands)}", end="", flush=True)
        print()

        # merge all ipcg graphs of all targets into one whole-program graph,
        # skipping graphs that were not created, e.g. due to a timeout
        createdIPCGs: list[str] = [ipcg for ipcg in tempIPCGs if os.path.isfile(ipcg)]
        subprocess.run([parserObject.cgmerge, parserObject.output] + createdIPCGs,stdout=outStream,stderr=errStream)


if __name__ == '__main__':
    main()