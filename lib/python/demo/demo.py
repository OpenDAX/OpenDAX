import pydax

pydax.init("PyDAX")

new_type = (("Mem1", "INT", 10),
            ("Mem2", "BOOL", 23),
            ("Mem3", "DINT", 3))

x = pydax.cdt_create("PyDAX_Type", new_type)
print hex(x)

pydax.add("PyDAXTAG", "INT", 10)
pydax.add("PyCDTTAG", "PyDAX_TYPE", 1)

print pydax.get("PyDAXTAG")
print pydax.get(0)
print pydax.get(1)
print pydax.get(2)

print pydax.cdt_get("PyDAX_Type")

pydax.free()
