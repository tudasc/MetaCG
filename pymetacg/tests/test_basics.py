import pytest

import pymetacg

def test_empty_cg(resource_dir):
    path = (resource_dir / "empty_cg.mcg").as_posix()
    cg = pymetacg.Callgraph.from_file(path)

    assert len(list(cg)) == 0
    assert "main" not in cg

def test_node_access(resource_dir):
    path = (resource_dir / "basic_cg.mcg").as_posix()
    cg = pymetacg.Callgraph.from_file(path)

    # test `in` functionality (`__contains__`)
    assert "main" in cg
    assert "foo" in cg
    assert "bar" in cg
    assert "notinhere" not in cg

    # test iterating functionality (`__iter__`)
    assert all(type(node) == pymetacg.CgNode for node in cg)
    assert set(node.function_name for node in cg) == {"main", "foo", "bar"}

    # test access (`__getitem__`)
    main = cg["main"]
    with pytest.raises(KeyError):
        cg["notinhere"]

    # test node equality (`__eq__`)
    assert cg["main"] == cg["main"]
    assert cg["main"] != cg["foo"]

def test_edges(resource_dir):
    path = (resource_dir / "basic_cg.mcg").as_posix()
    cg = pymetacg.Callgraph.from_file(path)

    main = cg["main"]
    foo = cg["foo"]
    bar = cg["bar"]


    assert main.callers == set()
    assert main.callees == {foo, bar}

    assert foo.callers == {main}
    assert foo.callees == {bar}

    assert bar.callers == {main, foo, bar}
    assert bar.callees == {bar}
