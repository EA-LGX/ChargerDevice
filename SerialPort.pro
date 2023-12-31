QT += core gui widgets serialport network mqtt
requires(qtConfig(combobox))

TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    settingsdialog.cpp \
    console.cpp \
    battery.cpp 

HEADERS += \
    mainwindow.h \
    settingsdialog.h \
    console.h \
    MyDialog.h \
    battery.h

FORMS += \
    mainwindow.ui \
    settingsdialog.ui 

RESOURCES += \
    SerialPort.qrc

#target.path = $$[QT_INSTALL_EXAMPLES]/serialport/terminal
#INSTALLS += target


#DESTDIR         = $$PWD/../app_bin
#MOC_DIR         = $$PWD/../build/serialport
#OBJECTS_DIR     = $$PWD/../build/serialport
