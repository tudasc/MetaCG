# File: test_basics.py
# License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at
# https://github.com/tudasc/metacg/LICENSE.txt

import pytest

import pymetacg

def test_version_info():
    assert "MetaCG" in pymetacg.info

def test_empty_cg(empty_cg):
    assert len(list(empty_cg)) == 0
    assert "main" not in empty_cg

def test_node_access(basic_cg):
    # test `in` functionality (`__contains__`)
    assert "main" in basic_cg
    assert "foo" in basic_cg
    assert "bar" in basic_cg
    assert "notinhere" not in basic_cg

    # test iterating functionality (`__iter__`)
    assert all(type(node) == pymetacg.CgNode for node in basic_cg)
    assert set(node.function_name for node in basic_cg) == {"main", "foo", "bar"}

    # test access (`__getitem__`)
    main = basic_cg["main"]
    with pytest.raises(KeyError):
        basic_cg["notinhere"]

    # test node equality (`__eq__`)
    assert basic_cg["main"] == basic_cg["main"]
    assert basic_cg["main"] != basic_cg["foo"]

def test_edges(basic_cg):
    main = basic_cg["main"]
    foo = basic_cg["foo"]
    bar = basic_cg["bar"]

    assert main.callers == set()
    assert main.callees == {foo, bar}

    assert foo.callers == {main}
    assert foo.callees == {bar}

    assert bar.callers == {main, foo, bar}
    assert bar.callees == {bar}

def test_file_format_v3(v3_cg):
    assert set(node.function_name for node in v3_cg) == {"foo", "main", "bar"}
    main = v3_cg["main"]
    foo = v3_cg["foo"]
    assert foo in main.callees
    assert main in foo.callers
