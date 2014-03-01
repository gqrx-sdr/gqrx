include config

INSTALL           = install -D

#all: deb

gqrx:
	mkdir -p ../../build
	sh -c "cd ../../build/; qmake ..; make || exit 1"
	test -f ../../build/gqrx

clean:
	$(RM) -r debian/gqrx/
	$(RM) debian/debhelper.log debian/gqrx.debhelper.log debian/gqrx.substvars debian/files

install-gqrx: gqrx
	$(INSTALL) ../../build/gqrx $(PREFIX)/bin/gqrx
	
install-freedesktop:
	$(INSTALL) ../../icons/scope.svg $(PREFIX)/share/pixmaps/gqrx.svg
	$(INSTALL) ../../gqrx.desktop $(PREFIX)/share/applications/gqrx.desktop
	
install: install-gqrx install-freedesktop

deb: gqrx
	@echo generating deb
	./gitlog-to-changelog > debian/changelog
	dpkg-buildpackage -b -rfakeroot; \
	cat debian/control | sed "s/^Version: \(.*\)/Version: auto/g" > debian/control.tmp && mv debian/control.tmp debian/control; \
	mkdir -p ./build/ && mv ../gqrx_* ./build/
	$(RM) build/*.changes
	@echo ".deb is located at build/"

rpm: deb
	fakeroot alien --keep-version --to-rpm build/*.deb && mv *.rpm build/

packages: deb rpm
