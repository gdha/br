%define rpmrelease %{nil}
%define bindir /usr/local/sbin

Name: br
Version: 1.6
Release: 1%{?dist}
Summary: Become root

Group: Applications/File
License: JNJ
URL: https://sourcecode.jnj.com/projects/SRV-001803/repos/lnxutils/
Source0: %{name}-%{version}.tar.gz
BuildRoot: %(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

BuildRequires: gcc
BuildRequires: autoconf
#Requires:	

%description
Become root allowed if you are member of specific group(s) mentioned in br header file


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
* Tue Jun 11 2024 Gratien D'haese (gdhaese1 @ its.jnj.com) - 1.6-1
- fix typo in gtscconf -> gtsccon
* Thu Jun  6 2024 Gratien D'haese (gdhaese1 @ its.jnj.com) - 1.5-1
- fix infinite loop in certain cases
* Fri Apr 12 2024 Gratien D'haese (gdhaese1 @ its.jnj.com) - 1.4-1
- add header file br.h containing allow_groups array
* Wed Mar 27 2024 Gratien D'haese (gdhaese1 @ its.jnj.com) - 1.3-1
- add setenv HOME and granted 'gtsccon' group
* Sat Mar  2 2024 Gratien D'haese (gdhaese1 @ its.jnj.com) - 1.2-1
- added setenv TERM
* Fri Mar  1 2024 Gratien D'haese (gdhaese1 @ its.jnj.com) - 1.1-1
- improved code with error checks and added 'wheel' group check
* Wed Feb 28 2024 Gratien D'haese (gdhaese1 @ its.jnj.com) - 1.0-1
- initial release (quick & dirty)
