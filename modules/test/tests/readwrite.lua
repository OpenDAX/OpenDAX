--These tests are to make sure that we are reading and writing data to
--and from the server correctly.

function print_table(table, indent)
  local indentstr = ""

  for n=0,indent-1 do
    indentstr = indentstr .. " "
  end

  for t,v in pairs(table) do
    if type(v)=="table" then
      print(indentstr .. t)
      print_table(v, indent+2)
    else
      print(indentstr .. t .." = " .. tostring(v))
    end
  end
end


function CheckArray(name, x, count)
  tag_write(name, x)
  
  y = tag_read(name, 0)
  for n=1,count do
    --print(tostring(y[n]).." = "..tostring(x[n]))
    if x[n] ~= y[n] then
      error("Array Problem for "..name)
    end
  end
end


--We start by creating tags of every kind of base datatype.

types = {"BOOL", "BYTE", "SINT", "WORD", "INT", "UINT", "DWORD", "DINT", 
         "UDINT", "TIME", "REAL", "LWORD", "LINT", "ULINT", "LREAL"}


for n=1,15 do
  --print("RWTest"..types[n].."Array")
  tag_add("RWTest"..types[n], types[n], 1)
  tag_add("RWTest"..types[n].."Array", types[n], 10)
end

x = {}

x[1] = true
x[2] = false
x[3] = true
x[4] = false
x[5] = true
x[6] = false
x[7] = true
x[8] = false
x[9] = true
x[10] = false

--CheckArray("RWTestBOOLArray", x, 10)

x[1] = 1
x[2] = 2
x[3] = 3
x[4] = 5
x[5] = 8
x[6] = 13
x[7] = 22
x[8] = 35
x[9] = 57
x[10]= 92

--CheckArray("RWTestBYTEArray", x, 10)

x[1] = -1
x[10]= -92
--CheckArray("RWTestSINTArray", x, 10)

x[1] = -1
x[2] = 2000
x[3] = 3001
x[4] = 5009
x[5] = 8000
x[6] = 1375
x[7] = 2233
x[8] = 3523
x[9] = 5799
x[10]= -92

--CheckArray("RWTestINTArray", x, 10)

x[1] = 12000
x[10]= 65535

--CheckArray("RWTestWORDArray", x, 10)
--CheckArray("RWTestUINTArray", x, 10)

--[[
CheckArray("RWTestDWORDArray", x, 10)
CheckArray("RWTestDINTArray", x, 10)
CheckArray("RWTestUDINTArray", x, 10)
CheckArray("RWTestTIMEArray", x, 10)
CheckArray("RWTestREALArray", x, 10)
CheckArray("RWTestLWORDArray", x, 10)
CheckArray("RWTestLINTArray", x, 10)
CheckArray("RWTestULINTArray", x, 10)
CheckArray("RWTestLREALArray", x, 10)
--]]

--This is where we start to read / write custom datatype tags


members = {{"Int",   "INT",   1},
           {"Dint",  "DINT",  1},
           {"Udint", "UDINT", 1}}
                                
test1 = cdt_create("RWTestSimple", members)

tag_add("RWTestSimple", test1, 1)
tag_add("RWTestSimpleArray", test1, 10)


members2 = {{"Real",       "REAL",          1},
            {"IntArray",   "INT",          10},
           -- {"RealArray",  "REAL",         10},
            {"Test",       "RWTestSimple",  1},
            {"TestArray",  "RWTestSimple", 10},
            {"Dint",       "DINT",          1}}

test2 = cdt_create("RWTest", members2)

tag_add("RWTest", test2, 1)
tag_add("RWTestArray", test2, 11)

tag_write("RWTestINT", 56)
print(tag_read("RWTestINT", 0))

x = {}

x.Int = 123
x.Dint = 456
x.Udint = 789

--tag_write("RWTestSimple", x)

--[[
y = tag_read("RWTestSimple", 0)

print(y)
print(y.Int)
print(y.Dint)
print(y.Udint)

if x.Int ~= y.Int then error("x.Int ~= y.Int") end
if x.Dint ~= y.Dint then error("x.Dint ~= y.Dint") end
if x.Udint ~= y.Udint then error("x.Udint ~= y.Udint") end

--]]

x = {}
x.Int = 12

--tag_write("RWTestSimple", x)

x = {}

for n=1,10 do
  x[n] = {}
  x[n].Int = n * 10 +1
  x[n].Dint = n * 10 +2
  x[n].Udint = n * 10 +3
end

tag_write("RWTestSimpleArray", x)
y = tag_read("RWTestSimpleArray", 0)

for n=1,10 do
  print("RWTestSimpleArray[" .. n .. "].Int = " .. tostring(y[n].Int))
  print("RWTestSimpleArray[" .. n .. "].Dint = " .. tostring(y[n].Dint))
  print("RWTestSimpleArray[" .. n .. "].Udint = " .. tostring(y[n].Udint))
end

x = {}

x.Real = 3.141592
x.IntArray = {}
x.IntArray[1] = 111
x.IntArray[2] = 222

x.Test = {}
x.Test.Int = 123
x.Test.Dint = 234
x.Test.Udint = 345

x.TestArray = {}

for n=1,10 do
  x.TestArray[n] = {}
  x.TestArray[n].Int = n*100 + n*10 + n
  x.TestArray[n].Dint = (n*100 + n*10 + n) * -1
  x.TestArray[n].Udint = n*1000 + n*100 + n*10 + n
end

tag_write("RWTest", x)

y = tag_read("RWTest", 0)

print("PRINTING TABLE")
print_table(y, 0)
print("DONE WITH THAT")

print("y.Real = " .. y.Real)

print(".IntArray[]")
for n=1,10 do
  print(tostring(x.IntArray[n]).." = "..tostring(y.IntArray[n]))
end

print(".Test.Int")
print(tostring(x.Test.Int).." = "..tostring(y.Test.Int))
print(".Test.Dint")
print(tostring(x.Test.Dint).." = "..tostring(y.Test.Dint))
print(".Test.Udint")
print(tostring(x.Test.Udint).." = "..tostring(y.Test.Udint))

print(".TestArray[].Int")
for n=1,10 do
  print(tostring(n))
  print("x.TestArray["..n.."].Int = "..tostring(x.TestArray[n].Int))
  print("y.TestArray["..n.."].Int = "..tostring(y.TestArray[n].Int))
end

--y = tag_read("RWTestSimple", 0)

--print(y.Int)

--tag_write("RWTestSimple.Int", 222)
--print(tag_read("RWTestSimple.Int", 0))


--[[
TODO:  Need to check for arrays within the datatypes, check for compound datatypes that contain
other compound datatypes as well as arrays of compound datatypes.  Also need to deal with things
that should make it fail, like missing members in the Lua variable as well as correctly named
members that are not tables when they should be or that are tables when they should not be.
--]]
