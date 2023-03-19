CFLAGS_INC += -I $(LIBDIR)/PLAT/inc

ifeq ($(TOOLCHAIN),ARMCC)
PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/libosa.lib \
                $(LIBDIR)/PLAT/lib/keil/libdriver_private.lib \
                $(LIBDIR)/PLAT/lib/keil/libmiddleware_ec_private.lib \
			
ifeq ($(THIRDPARTY_SOFTSIM_ENABLE),y)
PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/libsoftsim_apt.lib 
endif

ifeq ($(THIRDPARTY_SOFTSIM_CT_ENABLE),y)
PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/libesim_cos.lib \
				$(LIBDIR)/PLAT/lib/keil/libesim_ta.lib 
endif
endif

ifeq ($(TOOLCHAIN),GCC)
PREBUILDLIBS += $(LIBDIR)/PLAT/lib/keil/libosa.a \
                $(LIBDIR)/PLAT/lib/keil/libdriver_private.a \
                $(LIBDIR)/PLAT/lib/keil/libmiddleware_ec_private.a
endif