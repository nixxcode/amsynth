#  amsynth Makefile.am
#
#  Copyright (C) 2001 Nick Dowell
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

AM_CPPFLAGS = @CPPFLAGS@ -iquote "$(top_srcdir)/src" -iquote "$(top_srcdir)/external"

# http://www.gnu.org/software/libtool/manual/autoconf/Defining-Directories.html
AM_CPPFLAGS += -DDATADIR='"$(datadir)"' -DPKGDATADIR='"$(pkgdatadir)"' -DPACKAGE_LOCALEDIR=\""$(datadir)/locale"\"

AM_CFLAGS = $(WARN_CFLAGS) -Wno-declaration-after-statement -Wno-strict-prototypes -fPIC -fvisibility=hidden

AM_CXXFLAGS = $(WARN_CXXFLAGS) -fPIC -fvisibility=hidden

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	CHANGELOG.md \
	data \
	external/ardour/vestige.h \
	external/MTS-ESP/LICENSE \
	external/JUCE/LICENSE.md \
	external/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/lv2 \
	external/JUCE/modules/juce_core \
	external/JUCE/modules/juce_data_structures \
	external/JUCE/modules/juce_events \
	external/JUCE/modules/juce_graphics \
	external/JUCE/modules/juce_gui_basics

AM_CPPFLAGS += \
	-I$(top_srcdir)/external/JUCE/modules \
	-DJUCE_DISPLAY_SPLASH_SCREEN=0 \
	-DJUCE_GLOBAL_MODULE_SETTINGS_INCLUDED=1 \
	-DJUCE_INCLUDE_JPEGLIB_CODE=1 \
	-DJUCE_INCLUDE_PNGLIB_CODE=0 \
	-DJUCE_INCLUDE_ZLIB_CODE=0 \
	-DJUCE_MODULE_AVAILABLE_juce_core=1 \
	-DJUCE_MODULE_AVAILABLE_juce_data_structures=1 \
	-DJUCE_MODULE_AVAILABLE_juce_events=1 \
	-DJUCE_MODULE_AVAILABLE_juce_graphics=1 \
	-DJUCE_MODULE_AVAILABLE_juce_gui_basics=1 \
	-DJUCE_STRICT_REFCOUNTEDPOINTER=1 \
	-DJUCE_USE_CURL=0

SUBDIRS = po

################################################################################
#
# Core library
#

noinst_LTLIBRARIES = libcore.la

libcore_la_CPPFLAGS = $(AM_CPPFLAGS) @JUCE_CFLAGS@
libcore_la_LIBADD = @JUCE_LIBS@ -ldl
libcore_la_SOURCES = \
	external/freeverb/allpass.cpp \
	external/freeverb/allpass.hpp \
	external/freeverb/comb.cpp \
	external/freeverb/comb.hpp \
	external/freeverb/denormals.h \
	external/freeverb/revmodel.cpp \
	external/freeverb/revmodel.hpp \
	external/freeverb/tuning.h \
	external/JUCE/modules/juce_core/juce_core.cpp \
	external/JUCE/modules/juce_data_structures/juce_data_structures.cpp \
	external/JUCE/modules/juce_events/juce_events.cpp \
	external/JUCE/modules/juce_graphics/juce_graphics.cpp \
	external/JUCE/modules/juce_gui_basics/juce_gui_basics.cpp \
	external/seq24/controllers.h \
	src/core/Configuration.cpp \
	src/core/Configuration.h \
	src/core/controls.h \
	src/core/filesystem.cpp \
	src/core/filesystem.h \
	src/core/gettext.h \
	src/core/gui/ControlPanel.cpp \
	src/core/gui/ControlPanel.h \
	src/core/gui/Controls.cpp \
	src/core/gui/Controls.h \
	src/core/gui/JuceIntegration.cpp \
	src/core/gui/JuceIntegration.h \
	src/core/gui/LayoutDescription.h \
	src/core/gui/MainComponent.cpp \
	src/core/gui/MainComponent.h \
	src/core/midi.h \
	src/core/synth/ADSR.cpp \
	src/core/synth/ADSR.h \
	src/core/synth/Distortion.cpp \
	src/core/synth/Distortion.h \
	src/core/synth/LowPassFilter.cpp \
	src/core/synth/LowPassFilter.h \
	src/core/synth/MidiController.cpp \
	src/core/synth/MidiController.h \
	src/core/synth/Oscillator.cpp \
	src/core/synth/Oscillator.h \
	src/core/synth/Parameter.cpp \
	src/core/synth/Parameter.h \
	src/core/synth/Preset.cpp \
	src/core/synth/Preset.h \
	src/core/synth/PresetController.cpp \
	src/core/synth/PresetController.h \
	src/core/synth/SoftLimiter.cpp \
	src/core/synth/SoftLimiter.h \
	src/core/synth/Synth--.h \
	src/core/synth/Synthesizer.cpp \
	src/core/synth/Synthesizer.h \
	src/core/synth/TuningMap.cpp \
	src/core/synth/TuningMap.h \
	src/core/synth/VoiceAllocationUnit.cpp \
	src/core/synth/VoiceAllocationUnit.h \
	src/core/synth/VoiceBoard.cpp \
	src/core/synth/VoiceBoard.h \
	src/core/types.h

if BUILD_MTS_ESP
libcore_la_SOURCES += \
	external/MTS-ESP/Client/libMTSClient.cpp \
	external/MTS-ESP/Client/libMTSClient.h
endif

################################################################################
#
# amsynth standalone
#

bin_PROGRAMS = amsynth

amsynth_CPPFLAGS = $(AM_CPPFLAGS) @ALSA_CFLAGS@ @JACK_CFLAGS@ @LASH_CFLAGS@ @LIBLO_CFLAGS@
amsynth_LDADD = @ALSA_LIBS@ @JACK_LIBS@ @LASH_LIBS@ @LIBLO_LIBS@ @LIBS@ libcore.la
amsynth_SOURCES = \
	src/standalone/AudioOutput.cpp \
	src/standalone/AudioOutput.h \
	src/standalone/drivers/ALSAAudioDriver.cpp \
	src/standalone/drivers/ALSAAudioDriver.h \
	src/standalone/drivers/ALSAMidiDriver.cpp \
	src/standalone/drivers/ALSAMidiDriver.h \
	src/standalone/drivers/ALSAmmapAudioDriver.cpp \
	src/standalone/drivers/ALSAmmapAudioDriver.h \
	src/standalone/drivers/AudioDriver.h \
	src/standalone/drivers/MidiDriver.h \
	src/standalone/drivers/OSSAudioDriver.cpp \
	src/standalone/drivers/OSSAudioDriver.h \
	src/standalone/drivers/OSSMidiDriver.cpp \
	src/standalone/drivers/OSSMidiDriver.h \
	src/standalone/JackOutput.cpp \
	src/standalone/JackOutput.h \
	src/standalone/lash.c \
	src/standalone/lash.h \
	src/standalone/main.h \
	src/standalone/main.cpp

if BUILD_NSM
amsynth_SOURCES += \
	src/standalone/nsm/NsmClient.cpp \
	src/standalone/nsm/NsmClient.h \
	src/standalone/nsm/NsmHandler.cpp \
	src/standalone/nsm/NsmHandler.h \
	external/nsm/nsm.h
endif

################################################################################
#
# plugins
#
# using noinst prevents the .a and .la files being installed
# plugin gets installed via custom install-exec-hook
#

plugin_LDFLAGS = -avoid-version -module -disable-static

################################################################################
#
# DSSI
#

if BUILD_DSSI

dssidir = $(libdir)/dssi

# This links directly to .libs/libcore.a to avoid libtool's --whole-archive behaviour
# which is inappropriate for these targets - we don't want to include gui code in the
# plug-in, and don't want to include dsp code in the ui.

noinst_LTLIBRARIES += amsynth_dssi.la
amsynth_dssi_la_CXXFLAGS = @DSSI_CFLAGS@
amsynth_dssi_la_DEPENDENCIES = libcore.la
amsynth_dssi_la_LDFLAGS = $(plugin_LDFLAGS) -rpath $(dssidir) -export-symbols-regex "dssi_descriptor"
amsynth_dssi_la_LIBADD = .libs/libcore.a
amsynth_dssi_la_SOURCES = src/plugins/dssi/dssiplugin.cpp

endif

if BUILD_DSSI_GUI

noinst_PROGRAMS = amsynth_dssi_ui
amsynth_dssi_ui_CXXFLAGS = @DSSI_CFLAGS@ @LIBLO_CFLAGS@
amsynth_dssi_ui_LDADD = @DSSI_LIBS@ @LIBLO_LIBS@ libcore.la
amsynth_dssi_ui_SOURCES = src/plugins/dssi/dssiui.cpp

endif


################################################################################
#
# LV2
#

if BUILD_LV2

amsynth_lv2dir = $(libdir)/lv2/amsynth.lv2

# This links directly to .libs/libcore.a to avoid libtool's --whole-archive behaviour
# which is inappropriate for these targets - we don't want to include gui code in the
# plug-in, and don't want to include dsp code in the ui.

noinst_LTLIBRARIES += amsynth_lv2.la
amsynth_lv2_la_CXXFLAGS = -I"$(top_srcdir)/external/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/lv2"
amsynth_lv2_la_DEPENDENCIES = libcore.la
amsynth_lv2_la_LDFLAGS = $(plugin_LDFLAGS) -rpath $(amsynth_lv2dir) -export-symbols-regex "lv2_descriptor"
amsynth_lv2_la_LIBADD = .libs/libcore.a
amsynth_lv2_la_SOURCES = src/plugins/lv2/lv2plugin.cpp src/plugins/lv2/lv2plugin.h

noinst_LTLIBRARIES += amsynth_lv2ui.la
amsynth_lv2ui_la_CXXFLAGS = -I"$(top_srcdir)/external/JUCE/modules/juce_audio_processors/format_types/LV2_SDK/lv2"
amsynth_lv2ui_la_DEPENDENCIES = libcore.la
amsynth_lv2ui_la_LDFLAGS = $(plugin_LDFLAGS) -rpath $(amsynth_lv2dir) -export-symbols-regex "lv2ui_descriptor"
amsynth_lv2ui_la_LIBADD = .libs/libcore.a $(libcore_la_LIBADD)
amsynth_lv2ui_la_SOURCES = src/plugins/lv2/lv2ui.cpp

dist_amsynth_lv2_DATA = data/amsynth.lv2/amsynth.ttl data/amsynth.lv2/manifest.ttl

endif


################################################################################
#
# VST
#

if BUILD_VST

vstdir=$(libdir)/vst
noinst_LTLIBRARIES += amsynth_vst.la
amsynth_vst_la_LDFLAGS = $(plugin_LDFLAGS) -rpath $(vstdir) -export-symbols-regex "VSTPluginMain|^main"
amsynth_vst_la_LIBADD = libcore.la
amsynth_vst_la_SOURCES = src/plugins/vst2/vstplugin.cpp

endif


################################################################################
#
#

dist_pkgdata_DATA = data/rc

appicon48dir=$(datadir)/icons/hicolor/48x48/apps
appicon48_DATA=data/icons/48x48/amsynth.png

appiconsvgdir=$(datadir)/icons/hicolor/scalable/apps
appiconsvg_DATA=data/icons/scalable/amsynth.svg

desktopdir = $(datadir)/applications
desktop_in_files = data/amsynth.desktop.in
desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)

appdatadir = $(datadir)/appdata
appdata_in_files = data/amsynth.appdata.xml.in
appdata_in_files += data/dssi-amsynth-plugin.metainfo.xml.in
appdata_in_files += data/lv2-amsynth-plugin.metainfo.xml.in
appdata_in_files += data/vst-amsynth-plugin.metainfo.xml.in
appdata_DATA = $(appdata_in_files:.xml.in=.xml)
@INTLTOOL_XML_RULE@

EXTRA_DIST += $(appdata_in_files)

DISTCLEANFILES = $(appdata_DATA)

skinsdefaultdir="${pkgdatadir}/skins/default"
dist_skinsdefault_DATA = \
	data/skins/default/background.png \
	data/skins/default/button_simple.png \
	data/skins/default/filter_slope.png \
	data/skins/default/filter_type.png \
	data/skins/default/keybmode.png \
	data/skins/default/knob_boost_cut.png \
	data/skins/default/knob_boost.png \
	data/skins/default/knob_mix.png \
	data/skins/default/knob_osc_octave.png \
	data/skins/default/knob_osc_pitch.png \
	data/skins/default/knob_spot.png \
	data/skins/default/knob_width.png \
	data/skins/default/layout.ini \
	data/skins/default/osc_select.png \
	data/skins/default/portamento_modes.png \
	data/skins/default/waveform_lfo.png \
	data/skins/default/waveform_pics.png

banksdir = $(pkgdatadir)/banks
dist_banks_DATA = \
	data/banks/amsynth_factory.bank \
	data/banks/BriansBank01.amSynth.bank \
	data/banks/BriansBank02.amSynth.bank \
	data/banks/BriansBank03.amSynth.bank \
	data/banks/BriansBank04.amSynth.bank \
	data/banks/BriansBank05.amSynth.bank \
	data/banks/BriansBank06.amSynth.bank \
	data/banks/BriansBank07.amSynth.bank \
	data/banks/BriansBank08.amSynth.bank \
	data/banks/BriansBank09.amSynth.bank \
	data/banks/BriansBank10.amSynth.bank \
	data/banks/BriansBank11.amSynth.bank \
	data/banks/BriansBank12.amSynth.bank \
	data/banks/BriansBank13.amSynth.bank \
	data/banks/BriansBank14.amSynth.bank \
	data/banks/BriansBank15.amSynth.bank \
	data/banks/BriansBank16.amSynth.bank \
	data/banks/BriansBank17.amSynth.bank \
	data/banks/BriansBank18.amSynth.bank \
	data/banks/BriansBank19.amSynth.bank \
	data/banks/BriansBank20.amSynth.bank \
	data/banks/BriansBank21.amSynth.bank \
	data/banks/BriansBank22.amSynth.bank \
	data/banks/PatriksBank01.bank \
	data/banks/PatriksBank02.bank \
	data/banks/PatriksBank03.bank \
	data/banks/PatriksBank04.bank \
	data/banks/PatriksBank05.bank


################################################################################
#
# Man pages
#

if GENERATE_MAN_PAGES
SUBDIRS += man
endif


################################################################################
#
# Install & uninstall hooks
#

install-exec-hook:
if ENABLE_REALTIME
	chown root $(DESTDIR)$(bindir)/amsynth
	chmod   +s $(DESTDIR)$(bindir)/amsynth
endif
if BUILD_DSSI
	$(INSTALL_PROGRAM_ENV) $(INSTALL) -d $(DESTDIR)$(dssidir)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL) \
		$(top_builddir)/.libs/amsynth_dssi.so $(DESTDIR)$(dssidir)/amsynth_dssi.so
endif
if BUILD_DSSI_GUI
	$(INSTALL_PROGRAM_ENV) $(INSTALL) -d $(DESTDIR)$(dssidir)/amsynth_dssi
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL) \
		$(top_builddir)/amsynth_dssi_ui $(DESTDIR)$(dssidir)/amsynth_dssi/amsynth_dssi_ui
endif
if BUILD_LV2
	$(INSTALL_PROGRAM_ENV) $(INSTALL) -d $(DESTDIR)$(amsynth_lv2dir)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL) \
		$(top_builddir)/.libs/amsynth_lv2.so $(DESTDIR)$(amsynth_lv2dir)/amsynth_lv2.so
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL) \
		$(top_builddir)/.libs/amsynth_lv2ui.so $(DESTDIR)$(amsynth_lv2dir)/amsynth_lv2ui.so
endif
if BUILD_VST
	$(INSTALL_PROGRAM_ENV) $(INSTALL) -d $(DESTDIR)$(vstdir)
	$(INSTALL_PROGRAM_ENV) $(LIBTOOL) --mode=install $(INSTALL) \
		$(top_builddir)/.libs/amsynth_vst.so $(DESTDIR)$(vstdir)/amsynth_vst.so
endif

uninstall-hook:
	rm -rf $(DESTDIR)$(pkgdatadir)
if BUILD_DSSI
	rm  -f $(DESTDIR)$(dssidir)/amsynth_dssi.so
endif
if BUILD_DSSI_GUI
	rm -rf $(DESTDIR)$(dssidir)/amsynth_dssi
endif
if BUILD_LV2
	rm -rf $(DESTDIR)$(amsynth_lv2dir)
endif
if BUILD_VST
	rm  -f $(DESTDIR)$(vstdir)/amsynth_vst.so
endif
# Workaround for
# ERROR: files left in build directory after distclean:
# ./po/.intltool-merge-cache.lock
	-rm ./po/.intltool-merge-cache.lock


################################################################################
#
# Tests
#

check_PROGRAMS = amsynth-tests
amsynth_tests_LDADD = libcore.la
amsynth_tests_SOURCES = tests/tests.cpp

TESTS = $(check_PROGRAMS)
