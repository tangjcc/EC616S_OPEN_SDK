.PHONY:gccall
gccall:
	(cd project/$(TARGET)/apps/$(PROJECT)/GCC && $(MAKE) V=$(V) -f Makefile all)

.PHONY:clean-gccall
clean-gccall:
	(cd project/$(TARGET)/apps/$(PROJECT)/GCC && $(MAKE) -f Makefile clean)

.PHONY:keilall
keilall:
	(cd project/$(TARGET)/apps/$(PROJECT)/ARMCC && $(MAKE) V=$(V) -f Makefile all)

.PHONY:clean-keilall
clean-keilall:
	(cd project/$(TARGET)/apps/$(PROJECT)/ARMCC && $(MAKE) -f Makefile cleanall)
	
.PHONY:clean
clean:
	(cd project/$(TARGET)/apps/$(PROJECT)/ARMCC && $(MAKE) -f Makefile clean)

.PHONY:size
size:
	(cd project/$(TARGET)/apps/$(PROJECT)/GCC && $(MAKE) -f Makefile size)
