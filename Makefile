
PIDGIN_TREE_TOP ?= ../pidgin-2.14.12
PIDGIN3_TREE_TOP ?= ../pidgin-main
LIBPURPLE_DIR ?= $(PIDGIN_TREE_TOP)/libpurple
WIN32_DEV_TOP ?= $(PIDGIN_TREE_TOP)/../win32-dev

WIN32_CC ?= $(WIN32_DEV_TOP)/mingw-4.7.2/bin/gcc

PROTOC_C ?= protoc-c
PKG_CONFIG ?= pkg-config

DIR_PERM = 0755
LIB_PERM = 0755
FILE_PERM = 0644
# Note: Use "-C .git" to avoid ascending to parent dirs if .git not present
GIT_REVISION_ID = $(shell git -C .git rev-parse --short HEAD 2>/dev/null)
REVISION_ID = $(shell hg id -i 2>/dev/null)
REVISION_NUMBER = $(shell hg id -n 2>/dev/null)
ifneq ($(REVISION_ID),)
PLUGIN_VERSION ?= 0.10.$(shell date +%Y.%m.%d).git.r$(REVISION_NUMBER).$(REVISION_ID)
else ifneq ($(GIT_REVISION_ID),)
PLUGIN_VERSION ?= 0.10.$(shell date +%Y.%m.%d).git.$(GIT_REVISION_ID)
else
PLUGIN_VERSION ?= 0.10.$(shell date +%Y.%m.%d)
endif

CFLAGS	?= -O2 -g -pipe -Wall
LDFLAGS ?= -Wl,-z,relro

CFLAGS  += -std=c99 -DSTRINGNOTIFIER_PLUGIN_VERSION='"$(PLUGIN_VERSION)"'

# Comment out to disable localisation
CFLAGS += -DENABLE_NLS

# Do some nasty OS and purple version detection
ifeq ($(OS),Windows_NT)
  #only defined on 64-bit windows
  PROGFILES32 = ${ProgramFiles(x86)}
  ifndef PROGFILES32
    PROGFILES32 = $(PROGRAMFILES)
  endif
  STRINGNOTIFIER_TARGET = libstringnotifier.dll
  STRINGNOTIFIER_DEST = "$(PROGFILES32)/Pidgin/plugins"
  STRINGNOTIFIER_ICONS_DEST = "$(PROGFILES32)/Pidgin/pixmaps/pidgin/protocols"
  LOCALEDIR = "$(PROGFILES32)/Pidgin/locale"
else
  UNAME_S := $(shell uname -s)

  #.. There are special flags we need for OSX
  ifeq ($(UNAME_S), Darwin)
    #
    #.. /opt/local/include and subdirs are included here to ensure this compiles
    #   for folks using Macports.  I believe Homebrew uses /usr/local/include
    #   so things should "just work".  You *must* make sure your packages are
    #   all up to date or you will most likely get compilation errors.
    #
    INCLUDES = -I/opt/local/include -lz $(OS)

    CC = gcc
  else
    CC ?= gcc
  endif

  ifeq ($(shell $(PKG_CONFIG) --exists purple-3 2>/dev/null && echo "true"),)
    ifeq ($(shell $(PKG_CONFIG) --exists purple 2>/dev/null && echo "true"),)
      STRINGNOTIFIER_TARGET = FAILNOPURPLE
      STRINGNOTIFIER_DEST =
      STRINGNOTIFIER_ICONS_DEST =
    else
      STRINGNOTIFIER_TARGET = libstringnotifier.so
      STRINGNOTIFIER_DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=plugindir purple`
      STRINGNOTIFIER_ICONS_DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=datadir purple`/pixmaps/pidgin/protocols
      LOCALEDIR = $(DESTDIR)$(shell $(PKG_CONFIG) --variable=datadir purple)/locale
    endif
  else
    STRINGNOTIFIER_TARGET = libstringnotifier3.so
    STRINGNOTIFIER_DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=plugindir purple-3`
    STRINGNOTIFIER_ICONS_DEST = $(DESTDIR)`$(PKG_CONFIG) --variable=datadir purple-3`/pixmaps/pidgin/protocols
    LOCALEDIR = $(DESTDIR)$(shell $(PKG_CONFIG) --variable=datadir purple-3)/locale
  endif
endif

WIN32_CFLAGS = -std=c99 -I$(WIN32_DEV_TOP)/glib-2.28.8/include -I$(WIN32_DEV_TOP)/gtk_2_0-2.14/include -I$(WIN32_DEV_TOP)/glib-2.28.8/include/glib-2.0 -I$(WIN32_DEV_TOP)/glib-2.28.8/lib/glib-2.0/include -I$(WIN32_DEV_TOP)/json-glib-0.14/include/json-glib-1.0 -DENABLE_NLS -DSTRINGNOTIFIER_PLUGIN_VERSION='"$(PLUGIN_VERSION)"' -Wall -Wextra -Wno-deprecated-declarations -Wno-unused-parameter -fno-strict-aliasing -Wformat
WIN32_LDFLAGS = -L$(WIN32_DEV_TOP)/glib-2.28.8/lib -L$(WIN32_DEV_TOP)/gtk_2_0-2.14/lib -L$(WIN32_DEV_TOP)/json-glib-0.14/lib -lpurple -lintl -lglib-2.0 -lgobject-2.0 -ljson-glib-1.0 -g -ggdb -static-libgcc -lz
WIN32_PIDGIN2_CFLAGS = -I$(PIDGIN_TREE_TOP)/libpurple -I$(PIDGIN_TREE_TOP) $(WIN32_CFLAGS)
WIN32_PIDGIN3_CFLAGS = -I$(PIDGIN3_TREE_TOP)/libpurple -I$(PIDGIN3_TREE_TOP) -I$(WIN32_DEV_TOP)/gplugin-dev/gplugin $(WIN32_CFLAGS)
WIN32_PIDGIN2_LDFLAGS = -L$(PIDGIN_TREE_TOP)/libpurple $(WIN32_LDFLAGS)
WIN32_PIDGIN3_LDFLAGS = -L$(PIDGIN3_TREE_TOP)/libpurple -L$(WIN32_DEV_TOP)/gplugin-dev/gplugin $(WIN32_LDFLAGS) -lgplugin

CFLAGS += -DLOCALEDIR=\"$(LOCALEDIR)\"

C_FILES :=
PURPLE_COMPAT_FILES :=
PURPLE_C_FILES := string-notifier.c $(C_FILES)

.PHONY:	all install FAILNOPURPLE clean install-icons install-locales %-locale-install

LOCALES = $(patsubst %.po, %.mo, $(wildcard po/*.po))

all: $(STRINGNOTIFIER_TARGET)

libstringnotifier.so: $(PURPLE_C_FILES) $(PURPLE_COMPAT_FILES)
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -shared -o $@ $^ $(LDFLAGS) `$(PKG_CONFIG) purple glib-2.0 json-glib-1.0 --libs --cflags`  $(INCLUDES) -Ipurple2compat -g -ggdb

libstringnotifier3.so: $(PURPLE_C_FILES)
	$(CC) -fPIC $(CFLAGS) $(CPPFLAGS) -shared -o $@ $^ $(LDFLAGS) `$(PKG_CONFIG) purple-3 glib-2.0 json-glib-1.0 --libs --cflags` $(INCLUDES)  -g -ggdb

libstringnotifier.dll: $(PURPLE_C_FILES) $(PURPLE_COMPAT_FILES)
	$(WIN32_CC) -O0 -g -ggdb -shared -o $@ $^ $(WIN32_PIDGIN2_CFLAGS) $(WIN32_PIDGIN2_LDFLAGS) -Ipurple2compat

libstringnotifier3.dll: $(PURPLE_C_FILES) $(PURPLE_COMPAT_FILES)
	$(WIN32_CC) -O0 -g -ggdb -shared -o $@ $^ $(WIN32_PIDGIN3_CFLAGS) $(WIN32_PIDGIN3_LDFLAGS)

po/purple-stringnotifier.pot: libstringnotifier.c
	xgettext $^ -k_ --no-location -o $@

po/%.po: po/purple-stringnotifier.pot
	msgmerge $@ po/purple-stringnotifier.pot > tmp-$*
	mv -f tmp-$* $@

po/%.mo: po/%.po
	msgfmt -o $@ $^

%-locale-install: po/%.mo
	install -D -m $(FILE_PERM) -p po/$(*F).mo $(LOCALEDIR)/$(*F)/LC_MESSAGES/purple-stringnotifier.mo

install: $(STRINGNOTIFIER_TARGET) install-locales
	mkdir -m $(DIR_PERM) -p $(STRINGNOTIFIER_DEST)
	install -m $(LIB_PERM) -p $(STRINGNOTIFIER_TARGET) $(STRINGNOTIFIER_DEST)


install-locales: $(patsubst po/%.po, %-locale-install, $(wildcard po/*.po))

FAILNOPURPLE:
	echo "You need libpurple development headers installed to be able to compile this plugin"

clean:
	rm -f $(STRINGNOTIFIER_TARGET)

gdb:
	gdb --args pidgin -c ~/.fake_purple -n -m

