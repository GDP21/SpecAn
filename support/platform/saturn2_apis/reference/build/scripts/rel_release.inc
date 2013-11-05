###############################################
###############################################
##
##	release RELEASE Target 
##
##


CFLAGS += -D__RELEASE_RELEASE__ -D__RELEASE__=\"release\" -DNDEBUG

# GMAKE options
G_CFLAGS+=-DNDEBUG
