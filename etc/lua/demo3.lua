if _firstrun then
  x = 0
end

-- Global variables will survive executions
x = x+1
-- This is a global variable tied to a tag that will be updated
-- when the script finishes and reloaded the next time we run
demovar = demovar + 1

print(get_name(), x, demovar)

demoarr[1] = demovar
demoarr[7] = demovar + 1

if get_name() == "demo3a" then
   demoarr[8] = 3
end
