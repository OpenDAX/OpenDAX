--This is the main daxtest lua script.  The tests are each hard coded into
--the daxtest module, but the tests are selected and run from this Lua script

--Test that a certain number of tags can be added.
add_random_tags(1000, "Rand")


--This checks that the tagnames that should fail do so and that tags
--that should be allowed are allowed
fail = {"1Tag", "-Tag", "Tag-name", "Tag&name", "TagNameIsWayTooLong12345678912345"}

pass = {"_Tag", "Tag1", "tAg_name", "t1Ag_name", "TagNameIsBarelyLongEnoughToFit12"}

check_tagnames(fail, pass)
