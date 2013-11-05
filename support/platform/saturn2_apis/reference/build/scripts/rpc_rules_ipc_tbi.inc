###############################################
## Create $(GENSRC_TARGETS) from $(GEN_CLIENTSRC)
##
ifneq (,$(GEN_CLIENTSRC))
  INCLUDES += -I$(IMG_WORKROOT)/rpc/itc/include

  GENSRC_TARGETS = $(addprefix $(XGEN_TARGETDIR)/,$(GEN_CLIENTSRC:.h=_client.c))
  GEN_OBJECTS = $(addprefix $(XBUILDDIR)/,$(GEN_CLIENTSRC:.h=_client.o))
  SRCDIR = $(XGEN_TARGETDIR)/
##  $(warning **Generating: $(GENSRC_TARGETS)** )
endif

###############################################
## Create $(GENSRC_TARGETS) from $(GEN_SERVERSRC)
##
ifneq (,$(GEN_SERVERSRC))
  INCLUDES += -I$(IMG_WORKROOT)/rpc/itc/include

  GENSRC_TARGETS += $(addprefix $(XGEN_TARGETDIR)/,$(GEN_SERVERSRC:.h=_server.c))
  GEN_OBJECTS += $(addprefix $(XBUILDDIR)/,$(GEN_SERVERSRC:.h=_server.o))
  SRCDIR = $(XGEN_TARGETDIR)/
##  $(warning **Generating: $(GENSRC_TARGETS)** )
endif

###############################################
## .h -> _client.c rule
##
$(XGEN_TARGETDIR)/%_client.c: $(GEN_SRCDIR)%.h
	echo Generating client side code for $(<F);  \
	cp $(GEN_SRCDIR)$(<F) $(IMG_WORKROOT)/rpc/ipc_tbi/src/;
    ifneq (,$(GEN_DEPENDENT_HEADERS))
		cp $(GEN_DEPENDENT_HEADERS) $(IMG_WORKROOT)/rpc/ipc_tbi/src/;
    endif	
	STACKDIR=`pwd`; \
	cd $(IMG_WORKROOT)/rpc/ipc_tbi/; \
	$(IMG_WORKROOT)/bin/tools/win32/doxygen scripts/doxygen 2> $(XGEN_TARGETDIR)/doxygen.log; \
	$(TOOLSDIR)/xsltproc $(IMG_WORKROOT)/rpc/ipc_tbi/stylesheets/ipc_tbi_header.xsl xml/$(subst .,_8,$(subst _,__,$(<F))).xml > `cygpath -u $(IMG_WORKROOT)/rpc/itc/$(IMG_PROJ)/include/$(subst .h,_rpc.h,$(<F))`; \
	$(TOOLSDIR)/xsltproc $(IMG_WORKROOT)/rpc/ipc_tbi/stylesheets/ipc_tbi_client.xsl xml/$(subst .,_8,$(subst _,__,$(<F))).xml > $@; \
	rm -f $(IMG_WORKROOT)/rpc/ipc_tbi/src/$(<F);
    ifneq (,$(GEN_DEPENDENT_HEADERS))
		rm -f $(IMG_WORKROOT)/rpc/ipc_tbi/src/$(notdir $(GEN_DEPENDENT_HEADERS));
    endif	
	cd $(STACKDIR);

###############################################
## .h -> _server.c rule
##
$(XGEN_TARGETDIR)/%_server.c: $(GEN_SRCDIR)%.h
	echo Generating server side code for $(<F);  \
	cp $(GEN_SRCDIR)$(<F) $(IMG_WORKROOT)/rpc/ipc_tbi/src/;
    ifneq (,$(GEN_DEPENDENT_HEADERS))
		cp $(GEN_DEPENDENT_HEADERS) $(IMG_WORKROOT)/rpc/ipc_tbi/src/;
    endif
	STACKDIR=`pwd`; \
	cd $(IMG_WORKROOT)/rpc/ipc_tbi/; \
	$(IMG_WORKROOT)/bin/tools/win32/doxygen scripts/doxygen 2> $(XGEN_TARGETDIR)/doxygen.log; \
	$(TOOLSDIR)/xsltproc $(IMG_WORKROOT)/rpc/ipc_tbi/stylesheets/ipc_tbi_header.xsl xml/$(subst .,_8,$(subst _,__,$(<F))).xml > `cygpath -u $(IMG_WORKROOT)/rpc/itc/$(IMG_PROJ)/include/$(subst .h,_rpc.h,$(<F))`; \
	$(TOOLSDIR)/xsltproc $(IMG_WORKROOT)/rpc/ipc_tbi/stylesheets/ipc_tbi_server.xsl xml/$(subst .,_8,$(subst _,__,$(<F))).xml > $@; \
	rm -f $(IMG_WORKROOT)/rpc/ipc_tbi/src/$(<F);
    ifneq (,$(GEN_DEPENDENT_HEADERS))
		rm -f $(IMG_WORKROOT)/rpc/ipc_tbi/src/$(notdir $(GEN_DEPENDENT_HEADERS));
    endif	
	cd $(STACKDIR);
