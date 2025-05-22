# File: test_metadata.py
# License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
# https://github.com/tudasc/metacg/LICENSE.txt

import pytest

import pymetacg

def test_no_metadata(metadata_cg):
    n = metadata_cg["nodewithoutmetadata"]
    assert n.meta_data == {}

def test_builtin_metadata(metadata_cg):
    n = metadata_cg["nodewithbuiltinmetadata"]
    assert "numStatements" in n.meta_data
    md = n.meta_data["numStatements"]
    assert md.key == "numStatements"
    assert md.data == 42

def test_custom_metadata(metadata_cg):
    n = metadata_cg["nodewithcustommetadata"]
    assert "dummy_md" in n.meta_data
    md = n.meta_data["dummy_md"]
    assert md.key == "dummy_md"
    assert md.data == {"key1": "some string", "key2": 42}
