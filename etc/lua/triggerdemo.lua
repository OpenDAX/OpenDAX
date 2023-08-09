if _firstrun then
    print(get_name().." First Run")
    tag_add(get_name(), "DINT", 1)
else
    -- we can retrieve the data that is associated with the event
    -- that triggered the script in _trigger_data
    print(get_name().." data = "..tostring(_trigger_data))
end
