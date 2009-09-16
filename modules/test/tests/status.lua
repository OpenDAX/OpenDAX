--This is a corner case test to make sure that we can retrieve the first tag added
--to the system which is '_status'

--tag_add("_Tag", "DINT", 1)
--tag_add("Tag1", "DINT", 1)
--tag_add("tAg_name", "DINT", 1)
--tag_add("t1Ag_name", "DINT", 1)
--tag_add("TagNameIsBarelyLongEnoughToFit12", "DINT", 1)
--tag_add("StatusTest2", "BOOL", 1)
--tag_add("StatusTest3", "INT", 3)

tag_get("_status")
tag_get(0)

