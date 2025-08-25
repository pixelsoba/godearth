HEADERS += \
    tilesources/elevationtilesource.h \
	tilesources/localtilesource.h \
    tilesources/networktilesource.h \
    tilesources/tilepromise.h \
    tilesources/tilesource.h \
    tilesources/tilesourceimpl.h \
    tilesources/tmstilesource.h \
    tilesources/xyz.h

SOURCES += \
    tilesources/elevationtilesource.cpp \
	tilesources/localtilesource.cpp \
    tilesources/tilesource.cpp \
    tilesources/tmstilesource.cpp

include(vendors/vendors.pri)
