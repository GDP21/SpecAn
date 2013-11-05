###############################################
###############################################
##
##	test RELEASE Target 
##
##

CFLAGS += -D__RELEASE_TEST__ -D__RELEASE__=\"test\"

# GMAKE options
G_CFLAGS+=
export DEBUG=1
export METACOPT=2
