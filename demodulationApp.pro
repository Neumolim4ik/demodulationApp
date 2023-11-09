QT       += core gui
QT += multimedia
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    main.cpp \
    mainwindow.cpp

HEADERS += \
    mainwindow.h

FORMS += \
    mainwindow.ui

INCLUDEPATH +="D:/Projects/libsndfile/include"
LIBS += "D:/Projects/libsndfile/lib/sndfile.lib"
LIBS += "D:/Projects/libsndfile/bin/sndfile.dll"
INCLUDEPATH += D:/Projects/fftw/
QMAKE_LIBDIR += D:/Projects/fftw/
LIBS += -LD:/Projects/fftw/ -llibfftw3-3
LIBS += -LD:/Projects/fftw/ -llibfftw3f-3
LIBS += -LD:/Projects/fftw/ -llibfftw3l-3

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
