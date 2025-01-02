
AM_CFLAGS                       = -Wall -Wextra -O2

bin_PROGRAMS                    = br
br_SOURCES                      = br.c br.h br.8.adoc
br_LDADD                        =
dist_man_MANS                   = br.8
ACLOCAL_AMFLAGS                 = -I libltdl/m4
EXTRA_DIST                      = br.spec

if USE_ASCIIDOC
# begin of USE_ASCIIDOC

# asciidoc is in Python and used on Fedora and RHEL and cannot generate manpages, so we use xmlto to convert docbooc to manpage
asciidoc = asciidoc

man: br.8

br.8.xml: br.8.adoc
	$(asciidoc) -b docbook -d manpage $<

br.8: br.8.xml
	xmlto man $<
endif
# end of USE_ASCIIDOC

# Add targets to build rpm package
rpm: all dist $(PACKAGE).spec man
	@echo -e "\033[1m== Building RPM package $(PACKAGE)-$(PACKAGE_VERSION) ==\033[0;0m"
	rpmbuild -ta --clean \
		--define "_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm" \
		--define "debug_package %{nil}" \
		--define "_rpmdir %(pwd)" $(PACKAGE)-$(PACKAGE_VERSION).tar.gz

