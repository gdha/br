
AM_CFLAGS                       = -Wall -Wextra -O2
# Use $$2 (not $2) because make consumes one $ before invoking awk.
VERSION                         = $(shell awk -F'"' '/^#define VERSION / {print $$2; exit}' $(srcdir)/br.h)

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
else
man:
	@test -f br.8 || { \
		echo "error: br.8 is missing and asciidoc/xmlto are unavailable to rebuild it"; \
		exit 1; \
	}
endif
# end of USE_ASCIIDOC

# Add targets to build rpm package
rpm: all dist $(PACKAGE).spec man
	@echo -e "\033[1m== Building RPM package $(PACKAGE)-$(PACKAGE_VERSION) ==\033[0;0m"
	rpmbuild -ta --clean \
		--define "_rpmfilename %%{NAME}-%%{VERSION}-%%{RELEASE}.%%{ARCH}.rpm" \
		--define "debug_package %{nil}" \
		--define "_rpmdir %(pwd)" $(PACKAGE)-$(PACKAGE_VERSION).tar.gz

# Keep debian/br.dsc version metadata synchronized with br.h VERSION.
deb: all dist man
	@echo -e "\033[1m== Building DEB package $(PACKAGE)-$(PACKAGE_VERSION) ==\033[0;0m"
	sed -i -E "s/^Version: .*/Version: $(VERSION)/" debian/br.dsc
	sed -i -E "s|( br-)[^ ]+(\\.tar\\.gz)|\\1$(VERSION)\\2|" debian/br.dsc
	chmod 755 debian/rules
	fakeroot debian/rules clean
	fakeroot debian/rules build
	fakeroot debian/rules binary
	fakeroot dh_install
	-rm -rf debian/br
