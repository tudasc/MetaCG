# File: conftest.py
# License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
# https://github.com/tudasc/metacg/LICENSE.txt

import pytest

import pathlib

import pymetacg

@pytest.fixture(scope="session")
def resource_dir():
    return pathlib.Path(__file__).resolve().parent / "resources"
    
def read_mcg_file(resource_dir, filename):
    path = (resource_dir / filename).as_posix()
    cg = pymetacg.Callgraph.from_file(path)
    return cg

@pytest.fixture()
def empty_cg(resource_dir):
    return read_mcg_file(resource_dir, "empty_cg.mcg")

@pytest.fixture()
def basic_cg(resource_dir):
    return read_mcg_file(resource_dir, "basic_cg.mcg")

@pytest.fixture()
def metadata_cg(resource_dir):
    return read_mcg_file(resource_dir, "metadata_cg.mcg")

@pytest.fixture()
def v4_cg(resource_dir):
    return read_mcg_file(resource_dir, "basic_cg_v4.mcg")

@pytest.fixture()
def double_name_cg(resource_dir):
    return read_mcg_file(resource_dir, "double_name_cg.mcg")
