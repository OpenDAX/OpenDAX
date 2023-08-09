-- _firstrun is only true the first time the script is called
if _firstrun then
  print("First Run of Demo 1")
  --The name of the script configured in daxlua.conf
  print(get_name())
  --The full path to the script file
  print(get_filename())
end

-- Get the number of times that this script has executed
--print("Demo 1", get_executions() )

-- Get the times in uSec of the last time we ran and this time
-- print("Demo 1", get_lastscan() )
-- print("Demo 1", get_thisscan() )

-- This is the time since we ran last in uSec
print("Demo 1", get_interval() )

-- retrieve the id of another script given the name
id = get_script_id("demo2")

-- retrieve the name of the script given the id
-- scripts are indexed in the module starting with 
-- zero so this function can be used in a loop to find
-- all of the scripts.  It returns nil on bad index
name = get_script_name(id)

-- This function would cause us to be disabled and not run again
-- unless another script enabled us
-- disable_self()

-- These functions enable and disable other scripts given
-- either an id or a name.  Returns the index of the script
-- on success and nil on error.
-- A script could use these to disable themselves but there
-- is a race condition that occurs with the enable_trigger
-- event that is alleviated with the disable_self() function
--disable_script(name)
--enable_script(id)

