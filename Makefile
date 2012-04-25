# Makefile for Satana
# Works for GNU/Linux, not yet for MacOS X, not at all for Windows

INCLUDES  = -I.
LIBRARIES = -ldl -lm
CFLAGS    = $(INCLUDES) -Wall -Werror -g -O3 -fPIC
PLUGIN    = satana_4742
CC        = /usr/bin/cc
LD        = /usr/bin/ld
MD        = /bin/mkdir -p
INSTALL   = install -m 644

SYSTEM    = $(shell uname)

ifeq (${SYSTEM},Linux)
 LD = /usr/bin/ld -ldl -lm -g -shared
 INSTALL_PLUGINS_DIR = /usr/lib/ladspa/
else
ifeq (${SYSTEM},Darwin)
 LD = ${CC} -flat_namespace -undefined suppress -bundle -lbundle1.o -lmx -lm
 INSTALL_PLUGINS_DIR = /Library/Audio/Plug-ins/LADSPA
else
 LD = echo "NOT A SUPPORTED SYSTEM"
endif
endif

${PLUGIN}.so: ${PLUGIN}.c
	$(CC) $(CFLAGS) -o ${PLUGIN}.o -c ${PLUGIN}.c
	$(LD) -o ${PLUGIN}.so ${PLUGIN}.o 

${INSTALL_PLUGINS_DIR}:
	${MD} -p ${INSTALL_PLUGINS_DIR}

install: ${PLUGIN}.so ${INSTALL_PLUGINS_DIR}
	${INSTALL} ${PLUGIN}.so $(INSTALL_PLUGINS_DIR)

clean:
	-rm -f *.o *.so *~ core*

deinstall:
	-rm -f $(INSTALL_PLUGINS_DIR)${PLUGIN}.so

