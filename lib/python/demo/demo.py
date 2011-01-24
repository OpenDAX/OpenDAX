import pydax
import time

pydax.init("PyDAX")

new_type = (("Mem1", "INT", 10),
            ("Mem2", "INT", 1),
            ("Mem3", "DINT", 1))

x = pydax.cdt_create("PyDAX_Type", new_type)
#print hex(x)

pydax.add("PyBYTE", "BYTE", 10)
pydax.add("PySINT", "INT", 10)
pydax.add("PyINT", "INT", 10)
pydax.add("PyINT", "INT", 10)
pydax.add("PyBOOL", "BOOL", 10)

pydax.add("PyCDTTAG", "PyDAX_TYPE", 1)

print pydax.read("PyCDTTAG", 0)

#print pydax.get("PyDAXTAG")
#print pydax.get(0)
#print pydax.get(1)
#print pydax.get(2)

#print pydax.cdt_get("PyDAX_Type")

#print pydax.read("PyBYTE", 0)
#print pydax.read("PyBOOL[0]", 1)
#print pydax.read("PyBOOL", 0)


pydax.free()
