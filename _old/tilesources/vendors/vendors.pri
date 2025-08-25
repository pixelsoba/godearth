HEADERS += \
    tilesources/vendors/geoservices.h \
    tilesources/vendors/arcgisonline.h \
    tilesources/vendors/modestmap.h \
    tilesources/vendors/osm.h \
    tilesources/vendors/stamen.h \
    tilesources/vendors/mapbox.h


SOURCES += \
    tilesources/vendors/geoservices.cpp \
    tilesources/vendors/arcgisonline.cpp \
    tilesources/vendors/modestmap.cpp \
    tilesources/vendors/osm.cpp \
    tilesources/vendors/stamen.cpp \
    tilesources/vendors/mapbox.cpp


CONFIG(has_jawg_token) {
    DEFINES += MAPWIDGET_JAWG_TOKEN=$$(MAPWIDGET_JAWG_TOKEN)

    HEADERS += \
        tilesources/vendors/jawg.h
    
    SOURCES += \
        tilesources/vendors/jawg.cpp
}
