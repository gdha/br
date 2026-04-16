%define rpmrelease %{nil}
%define bindir /usr/local/sbin
%global br_version %(awk -F'"' '/^#define VERSION / { print $2; exit }' br.h)

Name: br
Version: %{br_version}
Release: 1%{?dist}
Summary: Become root

Group: Applications/File
License: GPLv3
URL: https://sourcecode.jnj.com/projects/SRV-001803/repos/lnxutils/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires: gcc
BuildRequires: autoconf
#Requires:	

%description
Become root allowed if you are member of specific group(s) mentioned
in br.h header file


%prep
%setup -q


%build
%configure --prefix=/usr/local
%make_build


%install
%{__rm} -rf %{buildroot}
#%make_install
# create directories
mkdir -vp \
        %{buildroot}%{bindir} %{buildroot}%{_mandir}/man8

# copy components into directories
cp -av %{name} %{buildroot}%{bindir}
chmod 4755 %{buildroot}%{bindir}/%{name}
cp -av %{name}.8 %{buildroot}%{_mandir}/man8/%{name}.8
gzip %{buildroot}%{_mandir}/man8/%{name}.8
chmod 0644 %{buildroot}%{_mandir}/man8/%{name}.8.gz

%clean
%{__rm} -rf %{buildroot}

%files
%defattr(-, root, root, 4755)
%{bindir}/br
%defattr(-, root, root, 0755)
%doc %{_mandir}/man8/br.8.gz


%changelog
* Thu Apr 16 2026 Gratien D'haese (gratien.dhaese @ gmail.com) - 1.7-1
- fix typo
