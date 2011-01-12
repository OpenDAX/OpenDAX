import pydax
import time



for n in xrange(1000000):
  x = pydax.test()
  print x

print 'Now We Sleep'
time.sleep(60)

