AM_CPPFLAGS = $(all_includes)
AM_CXXFLAGS = -Wreturn-type -ffast-math $(CXX_CPU_SWITCH)

liblinuxsamplereffectsincludedir = $(includedir)/linuxsampler/effects
liblinuxsamplereffectsinclude_HEADERS = \
	Effect.h \
	EffectInfo.h \
	EffectFactory.h \
	EffectChain.h \
	EffectControl.h

noinst_LTLIBRARIES = liblinuxsamplereffects.la
liblinuxsamplereffects_la_SOURCES = \
	Effect.cpp Effect.h \
	EffectInfo.h \
	EffectFactory.cpp EffectFactory.h \
	EffectChain.cpp EffectChain.h \
	EffectControl.cpp EffectControl.h \
	LadspaEffect.cpp LadspaEffect.h
