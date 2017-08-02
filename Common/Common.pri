TEMPLATE= app
CONFIG+= console
CONFIG-= app_bundle
CONFIG-= qt
CONFIG+= c++11

# For very old qmake, that can not recognize c++11 flag in config
QMAKE_CXXFLAGS+= -std=c++11
