###############################################
###############################################
##
##	Extra Meos Build rules 
## 
##	Required 'cos meos builds header files from xml source
## so MUST build meos before any code that uses it.


## keep 'intermediate' .h files in source directory.
## - 'cos they get copied to include dir, make assumes they
## are intermediate, so will normally delete them.

.PRECIOUS: $(SRCDIR)%.h

## if TARGETS contains .h
## Meos TARGETS always contain .h

ifneq (,$(filter %.h,$(TARGETS)))

## If NOT building qio (but are building other parts of meos)
## include these rules
## ie krn.h depends on dq.a and lst.a (via lst.h to prevent looping)

 ifeq (,$(filter $(SRCDIR)qio.h,$(TARGETS)))
  $(IMG_WORKROOT)/$(OSDEF)/$(OSVER)/include/lst.h: dqueues lists
  $(SRCDIR)krn.h: $(IMG_WORKROOT)/$(OSDEF)/$(OSVER)/include/lst.h
 endif

## Rules to make .h and .html files from .xml
## (used for meos headers)

$(SRCDIR)%.html: $(SRCDIR)%.xml
	cd .. ; $(XSLT) -IN $(^F) -XSL ../standard/doc.xsl -OUT $(@F) -HTML
	$(CP) $@ ../../../docs

$(SRCDIR)%.h:: $(SRCDIR)%.xml
	cd .. ; $(XSLT) -IN $(^F) -XSL ../standard/code.xsl -OUT $(@F) -TEXT
	$(CP) $@ ../../../include

endif
