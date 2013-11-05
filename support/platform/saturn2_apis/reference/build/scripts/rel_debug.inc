###############################################
###############################################
##
##	debug RELEASE Target 
##
##

CFLAGS += -D__RELEASE_DEBUG__ -D__RELEASE__=\"debug\"

# GMAKE options
G_CFLAGS+=
export DEBUG=1
