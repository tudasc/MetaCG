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
