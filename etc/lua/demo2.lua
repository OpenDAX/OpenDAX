if _firstrun then
  print("First Run of Demo 2")
  
  --The name of the script configured in daxlua.conf
  print(_name)
  --The full path to the script file
  print(_filename)
else

  print("Demo 2 - demoflag = ", demoflag)
end
