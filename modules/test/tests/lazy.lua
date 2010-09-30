--This is the lazy programmer test.  This is just a place for development tests for times
--when new features are being added and the tests have just not been formalized.  Once a test
--is formalized and permanent it should be moved to it's own script and given it's own C
--function interface

--members = {{"Int", "INT", 1},
--           {"Dint", "DINT", 1},
--           {"Udint", "UDINT", 1}}
                                
--test1 = cdt_create("LazySimple", members)

--tag_add("LazyTag", "LazySimple", 1)
--tag_add("LazyArray", "LazySimple", 10)

function callback(x)
   print("Callback Officially Called with " .. x)
end

tag_add("LazyTag", "DINT", 10)
event_add("LazyTag[2]", 1, "CHANGE", 0, callback, 44)


tag_write("LazyTag[2]", 23)
--event_select(2000)
event_poll()
--lazy_test()

