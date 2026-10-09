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
from multiprocessing import Pool


# A wrapper function to allow thread pooling
# as cgcollector does not terminate on every input file,
# we allow for timeouts to happen, and inform the user,
# which cgcollector invocation timed out after how many seconds
def collector(command: tuple[int, list[str]]) -> None:
    timeout, arguments = command
    try:
        subprocess.run(arguments, timeout=timeout)
    except subprocess.TimeoutExpired:
        print(f"cgcollector timed out after {timeout} seconds: {shlex.join(arguments)}")

def generateAPI() -> None:
    # create the directories
    os.makedirs(parserObject.build_directory + "/.cmake/api/v1/query/", exist_ok=True)
    # create the file
    open(parserObject.build_directory + "/.cmake/api/v1/query/codemodel-v2", "a").close()
    # run cmake to generate the answers
    # we might need to pass additional parameters to cmake to match the original compile
    cmake_command: list[str] = ["cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON"] + shlex.split(parserObject.cmake_args)
    print("cd " + parserObject.build_directory + " && " + shlex.join(cmake_command))
    subprocess.run(cmake_command, cwd=parserObject.build_directory)

def loadTargetDescription(cmakeJsonTarget: Json) -> Json:
    with open(parserObject.build_directory + "/.cmake/api/v1/reply/" + cmakeJsonTarget["jsonFile"], 'r') as targetDescription:
        return json.load(targetDescription)

def getSources(cmakeJsonTarget: Json) -> tuple[list[str], list[str]]:
    jsonTargetDescription: Json = loadTargetDescription(cmakeJsonTarget)

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
    print(includeFlags)
    print(sourcePaths)
    return includeFlags, sourcePaths

def getDependencyTargets(cmakeJsonTarget: Json) -> list[Json]:
    jsonTargetDescription: Json = loadTargetDescription(cmakeJsonTarget)
    if "dependencies" not in jsonTargetDescription:
        return []
    dependencyIds = [dependency["id"] for dependency in jsonTargetDescription["dependencies"]]
    return [target for target in allCmakeTargets if target["id"] in dependencyIds]


if __name__ == '__main__':
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
                        required=True)
    parser.add_argument('-r', '--recursive', metavar="<bool>", default=False, type=lambda v: v.lower() in ('true', '1', 'yes', 'on'),
                        help='Also collect dependency targets')
    parser.add_argument('--ci-concurrent-suffix', metavar="<str>", type=str,
                        help='Suffix appended to output filename in concurrent CI runs', required=False, default="")

    # parse all arguments after the first one, as this is the name we got called by
    parserObject: argparse.Namespace = parser.parse_args(sys.argv[1:])

    # if we want to generate a new api query file (or do both)
    if parserObject.generate in ['api', 'both']:
        generateAPI()
    # if we want to generate a graph for the target
    if parserObject.generate in ['graph', 'both']:

        # Find the most up-to-date reply
        reply_dir : pathlib.Path = pathlib.Path(parserObject.build_directory + "/.cmake/api/v1/reply")
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
            worklist: list[Json] = getDependencyTargets(target)
            while worklist:
                dependency: Json = worklist.pop(0)
                if dependency["id"] in visitedIds:
                    continue
                visitedIds.add(dependency["id"])
                collectedTargets.append(dependency)
                worklist += getDependencyTargets(dependency)

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
            includeFlags, sourceFiles = getSources(collectedTarget)
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
        with Pool(parserObject.jobs) as p:
            p.map(collector, commands)

        # merge all ipcg graphs of all targets into one whole-program graph,
        # skipping graphs that were not created, e.g. due to a timeout
        createdIPCGs: list[str] = [ipcg for ipcg in tempIPCGs if os.path.isfile(ipcg)]
        subprocess.run([parserObject.cgmerge, parserObject.output] + createdIPCGs)
