--These tests are to make sure that we are reading and writing data to
--and from the server correctly.

--Recursive function for printing an entire table to the screen
--  Used for debugging only, not really needed for the test
function print_table(table, indent)
  local indentstr = ""

  for n=0,indent-1 do
    indentstr = indentstr .. " "
  end

  for t,v in pairs(table) do
    if type(v)=="table" then
      --print(indentstr .. t)
      print_table(v, indent+2)
    else
      print(indentstr .. t .." = " .. tostring(v))
    end
  end
end

--This takes the numbers in the array 'tests' and writes them to tag 'name'
--one at a time and then reads them back and compares to make sure that it
--was read back correctly.
function CheckSingle(name, tests)
  for t,v in ipairs(tests) do
    --print("Checking " .. name .. " = " .. v)
    tag_write(name, v)
    y = tag_read(name, 0)
    if y ~= v then error(name .. " Problem " .. y .." ~= " .. v) end
  end
end


function CheckArray(name, x)
  tag_write(name, x)
  
  y = tag_read(name, 0)
  for t,v in ipairs(x) do
    --print(tostring(y[t]).." = "..tostring(x[t]))
    if x[t] ~= y[t] then
      error("Array Problem for "..name)
    end
  end
end

--This function cycles through the output table and compares the values
--that it finds there with the values in the input table if they exist
--If all the values match it returns true otherwise false
function compare_tables(input, output)
  for t,v in ipairs(output) do
    if input[t] ~= nil then
      if type(v) =="table" then
        compare_tables(input[t], v)
      else
        print(t .. " = " .. tostring(v))
    end
  end
end

--We start by creating tags of every kind of base datatype.

types = {"BOOL", "BYTE", "SINT", "WORD", "INT", "UINT", "DWORD", "DINT", 
         "UDINT", "TIME", "REAL", "LWORD", "LINT", "ULINT", "LREAL"}

for n=1,15 do
  tag_add("RWTest"..types[n], types[n], 1)
  tag_add("RWTest"..types[n].."Array", types[n], 10)
end


--This is where we test all the single tags.  This test checks the bounds of
--each data type and will tell us if we have any data formatting problems
function CheckSingles()
    --Single BOOL tag test
    tag = "RWTestBOOL"
    x = true
    tag_write(tag, x)
    y = tag_read(tag, 0)
    if y == false then error(tag .. " Problem") end
    x = false
    tag_write(tag, x)
    y = tag_read(tag, 0)
    if y == true then error(tag .. " Problem") end
    
    --Single integer type checks are easy enough
    tests = {255, 127, 0, 127, 255}
    CheckSingle("RWTestBYTE", tests)
    
    tests = {-128, 0, 127, 64, -1}
    CheckSingle("RWTestSINT", tests)
    
    tests = {65535, 0, 34000, 12, 65535}
    CheckSingle("RWTestWORD", tests)
    CheckSingle("RWTestUINT", tests)
    
    tests = {-32768, 0, 32767, -12000, -1}
    CheckSingle("RWTestINT", tests)
    
    tests = { 2^31 * -1, 2^31 -1, 0, -1}
    CheckSingle("RWTestDINT", tests)
    
    tests = { 2^32-1, 0, 2^31 - 2}
    CheckSingle("RWTestDWORD", tests)
    CheckSingle("RWTestUDINT", tests)
    CheckSingle("RWTestTIME", tests)
    
    tests = {2^63 * -1, 0, 2^62, -1}
    CheckSingle("RWTestLINT", tests)
    
    tests = {2^62-1, 0, 1, 265364764}  --Can't really check all 64 bits with Lua's number type
    CheckSingle("RWTestLWORD", tests)
    CheckSingle("RWTestULINT", tests)
    
    --Floating point checks are a bit trickier
    tests = {3.14159265358979, 2.345678E-23, 1.23446536E23, 0}
    name = "RWTestREAL"
    for t,v in ipairs(tests) do
      --print("Checking " .. name .. " = " .. v)
      tag_write(name, v)
      y = tag_read(name, 0)
      if math.abs(y - v) > (v * 0.01) then error(name .. " Problem " .. y .." ~= " .. v) end
    end
    
    --LREAL works well because this is really the same number that Lua uses internally
    tests = {3.14159265358979323846, 2.345678E-253, 1.2345734987E253, 0}
    name = "RWTestLREAL"
    for t,v in ipairs(tests) do
      --print("Checking " .. name .. " = " .. v)
      tag_write(name, v)
      y = tag_read(name, 0)
      if math.abs(y - v) > 0.0000000001 then error(name .. " Problem " .. y .." ~= " .. v) end
    end
end


function CheckArrays()
    --Now we check writing arrays of base datatypes
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
    
    CheckArray("RWTestBOOLArray", x)
    
    --This little test is to see if the masked writing works without messing up
    --the array elements between the ones that we want to change.
    
    xx = {}
    xx[2] = true
    x[2] = true
    xx[4] = true
    x[4] = true
    xx[7] = false
    x[7] = false
    
    tag_write("RWTestBOOLArray", xx)
    
    y = tag_read("RWTestBOOLArray", 0)
    
    for n=1,10 do
      if x[n] ~= y[n] then error("Masked write failed for RWTestBOOLArray at index " .. n) end
    end
    
    --Now check the integer arrays
    x[1] = 2
    x[2] = 0
    x[3] = 3
    x[4] = 5
    x[5] = 8
    x[6] = 13
    x[7] = 22
    x[8] = 255
    x[9] = 57
    x[10]= 92
    
    CheckArray("RWTestBYTEArray", x)
    
    x[1] = -1
    x[2] = -128
    x[8] = 25
    x[10]= 127
    
    CheckArray("RWTestSINTArray", x)
    
    x[1] = -1
    x[2] = 2000
    x[3] = 3001
    x[4] = 5009
    x[5] = 0
    x[6] = 1375
    x[7] = 32767
    x[8] = -32768
    x[9] = 5799
    x[10]= -92
    
    CheckArray("RWTestINTArray", x)
    
    x[1] = 1
    x[8] = x[8] * -1
    x[10]= 65535
    
    CheckArray("RWTestWORDArray", x)
    CheckArray("RWTestUINTArray", x)
    
    x[2] = 2^31 * -1
    x[10] = 2^31 -1
    
    CheckArray("RWTestDINTArray", x)
    
    x[2] = 2^32-1
    
    CheckArray("RWTestUDINTArray", x)
    CheckArray("RWTestDWORDArray", x)
    CheckArray("RWTestTIMEArray", x)
    
    --[[ TODO: Finish this
    CheckArray("RWTestLWORDArray", x)
    CheckArray("RWTestLINTArray", x)
    CheckArray("RWTestULINTArray", x)
    
    
    CheckArray("RWTestREALArray", x)
    CheckArray("RWTestLREALArray", x)
    --]]
    
    --This little test is to see if the masked writing works without messing up
    --the array elements between the ones that we want to change.
    for n=1,10 do
      x[n] = n
    end
    
    tag_write("RWTestINTArray", x)
    
    xx = {}
    xx[2] = 22
    x[2] = 22
    xx[4] = 44
    x[4] = 44
    tag_write("RWTestINTArray", xx)
    
    y = tag_read("RWTestINTArray", 0)
    
    for n=1,10 do
      if x[n] ~= y[n] then error("Masked write failed for RWTestINTArray at " .. x[n]) end
    end
end

CheckSingles()
CheckArrays()

--This is where we start to read / write custom datatype tags


members = {{"Int",   "INT",   1},
           {"Bool",  "BOOL",  1},
           {"reBool", "BOOL", 1},
           {"triBool", "BOOL",1},
           {"Dint",  "DINT",  1},
           {"Udint", "UDINT", 1}}
                                
test1 = cdt_create("RWTestSimple", members)

tag_add("RWTestSimple", test1, 1)
tag_add("RWTestSimpleArray", test1, 10)


members2 = {{"Real",       "REAL",          1},
            {"IntArray",   "INT",          10},
            {"RealArray",  "REAL",         10},
            {"Test",       "RWTestSimple",  1},
            {"TestArray",  "RWTestSimple", 10},
            {"Dint",       "DINT",          1}}

test2 = cdt_create("RWTest", members2)

tag_add("RWTest", test2, 1)
tag_add("RWTestArray", test2, 11)


print("STARTING CDT TEST")
x = {}

x.Int = 123
x.Bool = true
x.reBool = false
x.triBool = true
x.Dint = 456
x.Udint = 789


tag_write("RWTestSimple", x)
y = tag_read("RWTestSimple", 0)

print_table(y,0)


if x.Int ~= y.Int then error("x.Int ~= y.Int") end
if x.Bool ~= y.Bool then error("x.Bool ~= y.Bool") end
if x.reBool ~= y.reBool then error("x.reBool ~= y.reBool") end
if x.Dint ~= y.Dint then error("x.Dint ~= y.Dint") end
if x.Udint ~= y.Udint then error("x.Udint ~= y.Udint") end

x.Bool = true
x.reBool = true
x.triBool = true

tag_write("RWTestSimple", x)
y = tag_read("RWTestSimple", 0)

compare_tables(x,y)

print_table(y,0)

if x.Int ~= y.Int then error("x.Int ~= y.Int") end
if x.Bool ~= y.Bool then error("x.Bool ~= y.Bool") end
if x.reBool ~= y.reBool then error("x.reBool ~= y.reBool") end
if x.Dint ~= y.Dint then error("x.Dint ~= y.Dint") end
if x.Udint ~= y.Udint then error("x.Udint ~= y.Udint") end

tag_write("RWTestSimple.Bool", true)
y = tag_read("RWTestSimple.Bool", 0)
if y == false then error("Why ain't it true") end

tag_write("RWTestSimple.Bool", false)
y = tag_read("RWTestSimple.Bool", 0)
if y == true then error("Why ain't it false") end


print("TESTING, TESTING, TESTING")
--x = true
---tag_write("RWTestSimple.Bool", x)
---tag_write("RWTestSimple.reBool", x)
---tag_write("RWTestSimple.triBool", x)

x = {}
x.Bool = true
x.reBool = false
x.triBool = true

tag_write("RWTestSimple", x)


--y = tag_read("RWTestSimple", 0)
--if y.Bool ~= true then error ("No Match on RWTestSimple.Bool") end
--if y.reBool ~= true then error ("No Match on RWTestSimple.reBool") end
--if y.triBool ~= true then error ("No Match on RWTestSimple.triBool") end

tag_write("RWTestSimple.Bool", false)
tag_write("RWTestSimple.reBool", true)
tag_write("RWTestSimple.triBool", false)

y = tag_read("RWTestSimple", 0)
if y.Bool ~= false then error ("No Match on RWTestSimple.Bool") end
if y.reBool ~= true then error ("No Match on RWTestSimple.reBool") end
if y.triBool ~= false then error ("No Match on RWTestSimple.triBool") end



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

--[[
for n=1,10 do
  print("RWTestSimpleArray[" .. n .. "].Int = " .. tostring(y[n].Int))
  print("RWTestSimpleArray[" .. n .. "].Dint = " .. tostring(y[n].Dint))
  print("RWTestSimpleArray[" .. n .. "].Udint = " .. tostring(y[n].Udint))
end
--]]

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


--[[
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
--]]

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
