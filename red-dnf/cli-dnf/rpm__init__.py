# make sure to enforce redrpm usage
import sys
import redrpm

sys.modules['rpm'] = sys.modules['redrpm']
print ("**** Map Original LibRPM to RedRPM (should not be needed !!! ) ***")