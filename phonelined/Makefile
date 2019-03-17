# change this to set the installation prefix
PREFIX = /usr/local
# change this to change the set of runlevels on which phonelined should start
startlevels = 2 3
killlevels = 0 1 6

src = $(wildcard src/*.c)
obj = $(src:.c=.o)
dep = $(obj:.o=.d)

bin = phonelined


CFLAGS = -pedantic -Wall -g

$(bin): $(obj)
	$(CC) -o $@ $(obj) $(LDFLAGS)

-include $(dep)

%.d: %.c
	@echo depfile $@
	@$(CPP) $(CFLAGS) $< -MM -MT $(@:.d=.o) >$@

.PHONY: clean
clean:
	rm -f $(obj) $(bin)

.PHONY: cleandep
cleandep:
	rm -f $(dep)

.PHONY: install
install: $(bin)
	mkdir -p $(DESTDIR)$(PREFIX)/bin
	cp $(bin) $(DESTDIR)$(PREFIX)/bin/$(bin)
	cp init_script /etc/init.d/phoneline
	for i in $(startlevels); do \
		cd $(DESTDIR)/etc/rc$$i.d && ln -s ../init.d/phoneline S99phoneline; done
	for i in $(killlevels); do \
		cd $(DESTDIR)/etc/rc$$i.d && ln -s ../init.d/phoneline K01phoneline; done

.PHONY: uninstall
uninstall:
	rm -f $(DESTDIR)$(PREFIX)/bin/$(bin)
	rm -f $(DESTDIR)/etc/init.d/phoneline
	find $(DESTDIR)/etc/rc*.d -name '[SK]??phoneline' | xargs rm -f
