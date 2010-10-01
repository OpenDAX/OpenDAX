--This is the asynchronous event notification test.  It adds various tags of different
--types, sets up some events and then writes data to make sure that the events work
--as they are supposed to.

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

--First we check the write event

tag_add("EventDint", "DINT", 20)
event_add("EventDint[5]", 10, "WRITE", 0, callback, HIT)

EventTest("EventDint[4]",  22, MISS)
EventTest("EventDint[5]",  22, HIT)
EventTest("EventDint[14]", 22, HIT)
EventTest("EventDint[15]", 22, MISS)

tag_add("EventInt", "INT", 20)
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
