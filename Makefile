# Makefile for Satana
# Works for GNU/Linux, not yet for MacOS X, not at all for Windows

INCLUDES  = -I.
LIBRARIES = -ldl -lm
CFLAGS    = $(INCLUDES) -fno-common -Wall -Werror -g -O3 -fPIC -c
PLUGIN    = satana_4742
CC        = /usr/bin/cc
MD        = /bin/mkdir -p
INSTALL   = install -m 644

SYSTEM    = $(shell uname)

ifeq (${SYSTEM},Linux)
 LD = /usr/bin/ld -g -shared 
 INSTALL_PLUGINS_DIR = /usr/lib/ladspa/
else
 ifeq (${SYSTEM},Darwin)
  CFLAGS += -arch i386
  LD = ${CC} -flat_namespace -undefined suppress -bundle -lbundle1.o
  INSTALL_PLUGINS_DIR = /Library/Audio/Plug-ins/LADSPA
 else
  LD = echo "NOT A SUPPORTED SYSTEM"
 endif
endif

${PLUGIN}.so: ${PLUGIN}.c
	$(CC) $(CFLAGS) -o ${PLUGIN}.o ${PLUGIN}.c
	$(LD) ${LIBRARIES} -o ${PLUGIN}.so ${PLUGIN}.o 

${INSTALL_PLUGINS_DIR}:
	${MD} -p ${INSTALL_PLUGINS_DIR}

install: ${PLUGIN}.so ${INSTALL_PLUGINS_DIR}
	${INSTALL} ${PLUGIN}.so $(INSTALL_PLUGINS_DIR)

clean:
	-rm -f *.o *.so *~ core*

deinstall:
	-rm -f $(INSTALL_PLUGINS_DIR)${PLUGIN}.so

