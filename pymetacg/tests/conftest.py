import pytest

import pathlib

@pytest.fixture()
def resource_dir():
    return pathlib.Path(__file__).resolve().parent / "resources"
    