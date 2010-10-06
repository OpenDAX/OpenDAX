--This is the asynchronous event notification test.  It adds various tags of different
--types, sets up some events and then writes data to make sure that the events work
--as they are supposed to.

--TODO: These tests need to be formalized and completed

--Some definitions
HIT = 1
MISS = 0

lastevent = 0

--members = {{"Int", "INT", 1},
--           {"Dint", "DINT", 1},
--           {"Udint", "UDINT", 1}}
                                
--test1 = cdt_create("EventSimple", members)

function callback(x)
--   print("Lua Callback - lastevent = " .. x)
   lastevent = x
end

function EventTest(tagname, val, test)
    lastevent = MISS
    tag_write(tagname, val)
    event_poll()
    if test ~= lastevent then
        error("Event for " .. tagname .. " Failed")
    end
end    

tag_add("EventInt", "INT", 20)

e = event_add("EventInt[5]", 5, "EQUAL", 25, callback, HIT)

EventTest("EventInt[4]", 25, MISS)

EventTest("EventInt[5]", 24, MISS)
EventTest("EventInt[5]", 25, HIT)
EventTest("EventInt[5]", 25, MISS)
EventTest("EventInt[5]", 26, MISS)
EventTest("EventInt[5]", 25, HIT)

EventTest("EventInt[9]", 25, HIT)
EventTest("EventInt[10]", 25, MISS)

event_del(e)
e = event_add("EventInt[5]", 5, "GREATER", 25, callback, HIT)

EventTest("EventInt[4]", 5, MISS)
EventTest("EventInt[4]", 26, MISS)

EventTest("EventInt[5]", 24, MISS)
EventTest("EventInt[5]", 25, MISS)
EventTest("EventInt[5]", 26, HIT)
EventTest("EventInt[5]", 27, MISS)
EventTest("EventInt[5]", 25, MISS)
EventTest("EventInt[5]", 26, HIT)

EventTest("EventInt[9]", 26, HIT)
EventTest("EventInt[10]", 26, MISS)

event_del(e)

e = event_add("EventInt[5]", 5, "LESS", 25, callback, HIT)

EventTest("EventInt[4]", 5, MISS)
EventTest("EventInt[4]", 24, MISS)

EventTest("EventInt[5]", 24, HIT)
EventTest("EventInt[5]", 25, MISS)
EventTest("EventInt[5]", 26, MISS)
EventTest("EventInt[5]", 24, HIT)
EventTest("EventInt[5]", 25, MISS)
EventTest("EventInt[5]", 24, HIT)

EventTest("EventInt[9]", 24, HIT)
EventTest("EventInt[10]", 24, MISS)


--Boolean Change Events
tag_add("EventBOOL", "BOOL", 20)
arr = {}
for n=1,20 do
  arr[n] = false;
end


e = event_add("EventBOOL[3]", 9, "CHANGE", 0, callback, HIT)
EventTest("EventBOOL", arr, MISS)
arr[3] = true
EventTest("EventBOOL", arr, MISS)
arr[4] = true
EventTest("EventBOOL", arr, HIT)
arr[12] = true
EventTest("EventBOOL", arr, HIT)
arr[13] = true
EventTest("EventBOOL", arr, MISS)
--Delete the event when we are done
event_del(e)

--Boolean Set Events
for n=1,20 do
  arr[n] = false;
end
tag_write("EventBOOL", arr)
e = event_add("EventBOOL[3]", 9, "SET", 0, callback, HIT)

EventTest("EventBOOL", arr, MISS)
arr[3] = true
EventTest("EventBOOL", arr, MISS)
arr[4] = true
EventTest("EventBOOL", arr, HIT)
arr[12] = true
EventTest("EventBOOL", arr, HIT)
arr[13] = true
EventTest("EventBOOL", arr, MISS)
--Check that we only get it once for each time we set the bit
EventTest("EventBOOL[11]", true, MISS) --Already fired this one
EventTest("EventBOOL[11]", false, MISS)
EventTest("EventBOOL[11]", true, HIT)

--Delete the event when we are done
event_del(e)

--Boolean Reset Events
for n=1,20 do
  arr[n] = false;
end
tag_write("EventBOOL", arr)
e = event_add("EventBOOL[3]", 9, "RESET", 0, callback, HIT)

--Because some of the bits are already false and our mask in the event is 
--  all zeros the first write will fire the event.
EventTest("EventBOOL", arr, HIT)

--Check that we only get it once for each time we set the bit
EventTest("EventBOOL[11]", true, MISS)
EventTest("EventBOOL[11]", false, HIT)
EventTest("EventBOOL[11]", false, MISS)
EventTest("EventBOOL[11]", true, MISS)
EventTest("EventBOOL[11]", false, HIT)

--Writing all 1's shouldn't hit this message
for n=1,20 do
  arr[n] = true;
end
EventTest("EventBOOL", arr, MISS)
--check the range
arr[3] = false
EventTest("EventBOOL", arr, MISS)
arr[4] = false
EventTest("EventBOOL", arr, HIT)
arr[12] = false
EventTest("EventBOOL", arr, HIT)
arr[13] = false
EventTest("EventBOOL", arr, MISS)

--Delete the event when we are done
event_del(e)

--First we check the write event

tag_add("EventDint", "DINT", 20)
e = event_add("EventDint[5]", 10, "WRITE", 0, callback, HIT)

EventTest("EventDint[4]",  22, MISS)
EventTest("EventDint[5]",  22, HIT)
EventTest("EventDint[14]", 22, HIT)
EventTest("EventDint[15]", 22, MISS)

event_del(e)

event_add("EventInt[5]", 10, "CHANGE", 0, callback, HIT)

EventTest("EventInt[4]",  22, MISS)
EventTest("EventInt[5]",  22, HIT)
EventTest("EventInt[14]", 22, HIT)
EventTest("EventInt[15]", 22, MISS)
EventTest("EventInt[14]", 22, MISS)
EventTest("EventInt[14]", 23, HIT)

arr = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0} 
EventTest("EventInt", arr, HIT)
EventTest("EventInt", arr, MISS)

arr = {0, 1, 2, 3, 4, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 16, 17, 18, 19}
EventTest("EventInt", arr, MISS)
 
arr = {0, 1, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 16, 17, 18, 19}
EventTest("EventInt", arr, HIT)

arr = {0, 1, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 0, 15, 16, 17, 18, 19}
EventTest("EventInt", arr, MISS)

arr = {0, 1, 2, 3, 4, 5, 0, 0, 0, 0, 0, 0, 0, 0, 14, 15, 16, 17, 18, 19}
EventTest("EventInt", arr, HIT)
