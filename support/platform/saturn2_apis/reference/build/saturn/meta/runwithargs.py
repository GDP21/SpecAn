from CSUtils import DA
import sys

if __name__ == "__main__":
    # execute the existing ldlk generated script
    DA.LoadProgramFileEx(sys.argv[1])
    
    args = sys.argv[2:]
    
    thread = DA.GetFirstThread()
    
    while thread:

    	try:
    		DA.SelectTarget(thread)
    		argc = DA.EvaluateSymbol("metag_argc")
        	print("Patching %d arguments" % len(args))
        	DA.WriteLong("&metag_argc", len(args) + 1)
        	for n, arg in enumerate(args):
            		print("patching arg %r to metag_argv[%d]" % (arg, n+1))
            		DA.WriteString("metag_argv[%d]" % (n +1), arg)
            	break
    	except DA.Error as e:
    		thread = DA.GetNextThread(thread)
    
    if(not thread):
    	sys.exit("ERROR: Failed patching metag_argc/metag_argv : %s" % e)
