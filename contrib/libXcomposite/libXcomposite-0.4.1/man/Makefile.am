libmandir = $(LIB_MAN_DIR)

LIB_MAN_DIR_SUFFIX = $(LIB_MAN_DIR:@mandir@/man%=%)

libman_xml = Xcomposite.xml

libman_PRE = $(libman_xml:xml=man)

Xcomposite_man_aliases = 			\
	XCompositeQueryExtension		\
	XCompositeQueryVersion			\
	XCompositeVersion			\
	XCompositeRedirectWindow		\
	XCompositeRedirectSubwindows		\
	XCompositeUnredirectWindow		\
	XCompositeUnredirectSubwindows		\
	XCompositeCreateRegionFromBorderClip	\
	XCompositeNameWindowPixmap		\
	XCompositeGetOverlayWindow		\
	XCompositeReleaseOverlayWindow

Xcomposite_shadowmen = $(Xcomposite_man_aliases:=.@LIB_MAN_SUFFIX@)

libman_DATA = $(libman_PRE:man=@LIB_MAN_SUFFIX@) \
	      $(Xcomposite_shadowmen)

EXTRA_DIST  = $(libman_PRE) $(libman_xml)

CLEANFILES  = $(libman_DATA)

MAINTAINERCLEANFILES = $(libman_PRE)

# Generate man page shadow files (Replaces InstallManPageAliases from Imake)
$(Xcomposite_shadowmen):
	$(AM_V_GEN) echo .so man$(LIB_MAN_DIR_SUFFIX)/Xcomposite.$(LIB_MAN_SUFFIX) > $@

# Substitute system-specific values in man pages

XORGRELSTRING = @PACKAGE_STRING@
  XORGMANNAME = X Version 11

MAN_SUBSTS = \
	-e 's|"__vendorversion__"|"$(XORGRELSTRING)" "$(XORGMANNAME)"|' \
	-e 's|__xorgversion__|"$(XORGRELSTRING)" "$(XORGMANNAME)"|' \
	-e 's|__xservername__|Xorg|g' \
	-e 's|__xconfigfile__|xorg.conf|g' \
	-e 's|__projectroot__|$(prefix)|g' \
	-e 's|__apploaddir__|$(appdefaultdir)|' \
	-e 's|__appmansuffix__|$(APP_MAN_SUFFIX)|g' \
	-e 's|__libmansuffix__|$(LIB_MAN_SUFFIX)|g' \
	-e 's|__adminmansuffix__|$(ADMIN_MAN_SUFFIX)|g' \
	-e 's|__miscmansuffix__|$(MISC_MAN_SUFFIX)|g' \
	-e 's|__filemansuffix__|$(FILE_MAN_SUFFIX)|g'

SUFFIXES = .$(LIB_MAN_SUFFIX) .man .xml

.man.$(LIB_MAN_SUFFIX):
	$(AM_V_GEN)sed $(MAN_SUBSTS) < $< > $@

# Generate nroff sources from DocBook/xml

dist-hook: $(libman_PRE)

if HAVE_XMLTO
.xml.man:
	$(AM_V_GEN)$(XMLTO) man $< && mv $*.__libmansuffix__ $@
else
.xml.man:
	@echo "WARNING: configure did not find xmlto, cannot create $@ without it"
endif
