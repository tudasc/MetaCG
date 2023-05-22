#
# File: TargetCollector.py
# This utility script allows to generate the whole-program call-graph for a given cmake target
# Script Version: 0.6.0
# License: Part of the MetaCG project. Licensed under BSD 3 clause license. See LICENSE.txt file at
# https://github.com/tudasc/metacg/LICENSE.txt
#

import argparse
import sys
import subprocess
import os
import json
from multiprocessing import Pool


# A wrapper function to allow thread pooling
# as cgcollector does not terminate on every input file,
# we allow for timeouts to happen, and inform the user,
# which cgcollector invocation timed out after how many seconds
def collector(command):
    subprocess.run(command[1].split(" "), timeout=command[0])


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

    # parse all arguments after the first one, as this is the name we got called by
    parserObject: argparse.Namespace = parser.parse_args(sys.argv[1:])

    # if we want to generate a new api query file (or do both)
    if parserObject.generate in ['api', 'both']:
        # create the directories
        os.makedirs(parserObject.build_directory + "/.cmake/api/v1/query/", exist_ok=True)
        # create the file
        open(parserObject.build_directory + "/.cmake/api/v1/query/codemodel-v2", "a").close()
        # run cmake to generate the answers
        # we might need to pass additional parameters to cmake to match the original compile
        cmake_command: str = "cmake " + parserObject.cmake_args
        print("cd " + parserObject.build_directory + " && " + cmake_command)
        subprocess.run(cmake_command.split(" "), cwd=parserObject.build_directory)
    # if we want to generate a graph for the target
    if parserObject.generate in ['graph', 'both']:
        # we need to find the answer file for our target
        # for this, we list all targets, and find ours, via its prefix
        # target names are unique, meaning we get THE target with [0]
        targetFile: str = [f for f in os.listdir(parserObject.build_directory + "/.cmake/api/v1/reply") if
                           f.startswith("target-" + parserObject.target + "-")][0]

        # get the json from the file and extract the information we need
        # file is closed after json load finishes
        with open(parserObject.build_directory + "/.cmake/api/v1/reply/" + targetFile, 'r') as targetDescription:
            jsonFile: json = json.load(targetDescription)

        # get all include statements dictionaries from the compile group key
        includeInformation: dict = {}
        listOfDicts = [includeInformation.update(dict(elem)) for elem in jsonFile["compileGroups"]]
        # get the files from the include dictionary
        includeDirectories = [dict(elem)["path"] for elem in includeInformation["includes"]]

        # get all source files from the compile-sources
        # we are not interested in generating a callgraph for *.h files,
        # as they are included anyway
        sources = [dict(elem)["path"] for elem in jsonFile["sources"] if not dict(elem)["path"].endswith("h")]

        # if we are not using a compile statements database, we need to pass the include-directories to out tools
        # if the user wants to pass any other arguments, we pipe them along
        extraArguments = [f'--extra-arg=-I{i}' for i in includeDirectories] + \
                         [f'--extra-arg=-I{i}' for i in parserObject.extra_args.split(" ") if i != ""]

        # generate a separate cgcollector command for each source
        commands = [
            parserObject.cgcollector + " " + " ".join(extraArguments) + " " + source
            for source in sources]

        # use a thread pool to run all commands in parallel (w.r.t pool-size)
        with Pool(parserObject.jobs) as p:
            p.map(collector, [[parserObject.wallclock_timeout, elem] for elem in commands])

        # generate the command to merge all ipcg graphs
        command = parserObject.cgmerge + " " + parserObject.output + " " + " ".join(
            ['.'.join(source.split('.')[:-1]) + ".ipcg" for source in sources])

        # run the command
        subprocess.run(command.split(" "))
