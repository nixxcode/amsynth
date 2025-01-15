## [Unreleased]

  - Ported the GUI from GTK2 to JUCE.
    PACKAGERS TAKE NOTE: build dependencies have changed!
  - Added preset selection, saving, and main menu to plug-ins GUIs.
  - The lv2-dev package is no longer required; JUCE bundles the LV2 headers.
  - Added support for [MTS-ESP](https://github.com/ODDSound/MTS-ESP) microtuning,
    excluding multi-channel tuning tables.
  - Removed support for JACK-Session, which is deprecated and unsupported.
  - Fixed a memory leak in VST effGetChunk handler.
  - Fixed unintentional distortion on some presets - issue #235
  - Fixed values passed to VST audioMasterAutomate.
  - Added Visual Studio project to allow building VST for Windows.


## 1.13.4 (2024-05-02)

  - Fixed "reference to 'filesystem' is ambiguous" build error.


## 1.13.3 (2024-04-15)

  - Fixed an error when building DSSI GUI with GCC 14 or Clang.


## 1.13.2 (2023-02-01)

  - Fixed a packaging error that cause preset banks to be omitted.


## 1.13.1 (2023-01-21)

  - Fixed a crash when parsing XSettings on some systems.


## 1.13.0 (2022-07-16)

  - Fixed intermittent unexpected MIDI output in response to MIDI CC input.
  - Fixed VST plugin UI updates in response to host parameter changes.
  - Added HiDPI scaling support to VST plugin UI.
  - Added version info to plugin pop-up menu.


## 1.12.4 (2022-01-08)

  - Fixed lv2 .ttl syntax error


## 1.12.3 (2022-01-01)

  - Added PatriksBank04 and PatriksBank05.
  - Fixed click when playing a note for the first time - issue #196
  - Stopped vendoring lv2 headers; install lv2-dev (or similar) via your package manager.


## 1.12.2 (2020-11-20)

  - Fixed a regression in 1.12.0 that broke ALSA MIDI


## 1.12.1 (2020-11-14)

  - Fixed LV2 manifest format error


## 1.12.0 (2020-11-08)

  - Improved HiDPI autodetection and added --force-device-scale-factor command line option
  - Presets are now available to be loaded in VST hosts using the generic GUI (e.g. REAPER)
  - Fixed a bug that caused MIDI channel and polyphony settings from the command line or
    configuration file to be ignored.
  - Fixed a bug that caused MIDI channel setting to be ignored when sending MIDI over JACK
  - Fixed a MIDI parsing bug in the VST plugin that caused stuck notes in REAPER
  - Fixed a crash when compiled with LASH support but without a LASH server running


## 1.11.0 (2020-09-20)

  - Added mouse wheel support for controls
  - Implemented UI upscaling for background and controls on HiDPI displays
  - Fixed a regression in 1.10.0 that changed the sound of patches using ring modulation
  - Fixed LV2 lint error caused by missing minorVersion and microVersion
  - Removed dependency on oscpack for Non Session Manager support


## 1.10.0 (2020-05-07)

  - Implemented smoothing / de-zippering to improve sound quality while adjusting parameters
  - amsynth user files (config, banks, etc.) now live in XDG-compliant directories
    - $XDG_CONFIG_HOME (~/.config/amsynth)
    - $XDG_DATA_HOME (~/.local/share/amsynth)
  - Added `jack_autoconnect` configuration option
  - Disabled VST GUI in REAPER


## 1.9.0 (2019-04-13)

  - Added support for NSM (Non Session Manager)
  - Improved FreeBSD OSS audio support
  - Sustain pedal can now be used in Mono and Legato modes
  - JACK MIDI inputs are now automatically connected
  - Fixed a crash in MIDI learn dialog when receiving CC messages
  - Other minor bug fixes


## 1.8.0 (2017-06-25)

  - controls can now be reset to their default value via a double-click
  - alternate tuning scales & keyboard maps can now be loaded in plug-ins
  - fixed an issue where bank switching via MIDI caused jack zombification
  - removed dependency on gtkmm (gtk is still required for the UI)


## 1.7.1 (2017-03-15)

  - fixed MIDI channel filtering
  - fixed corrupt preset names in dssi plugin
  - general UI and menu item improvements
  - improvements to translations
  - added a man page


## 1.7.0 (2016-09-28)

  - MIDI CCs are now sent immediately when changing a parameter in the UI
  - fixed an issue with ALSA MIDI input becoming delayed
  - improved UI error detection / reporting when using JACK
  - implemented bank switching via MIDI (CC #0)
  - implemented "Ignore Preset Value" option to allow parameters to be "locked"
  - enabled use of alternate tunings in plugin
  - added French and German translations
  - added a manpage
  - fixed UI issues when used as a plugin in Tracktion or other JUCE based host
  - fixed some UI layout glitches and improved the "audition" UI
  - fixed use of deprecated snd_seq_free_event API


## 1.6.4 (2016-01-19)

  - fix incorrect parameter display when vst gui is created
  - support selection of a non-default tuning via command line or config file
  - another sound bank thanks to brian :)


## 1.6.3 (2015-10-31)

  - fixed vst load & save compatibility with Bitwig Studio
  - fixes a warning about start_atomic_value_change in plugin versions


## 1.6.2 (2015-10-24)

  - fixes an issue with MIDI event handling observed with Ardour/LV2


## 1.6.1 (2015-10-10)

  - fixes support for non-44100 sample rates in DSSI and LV2 builds


## 1.6.0 (2015-09-27)

  - VST plug-in build now available
  - adds a portamento mode control with legato option
  - can now be built without GUI (./configure --without-gui)
  - fixes an issue with incorrect tuning depending on sample rate
  - other minor bug fixes and improvements


## 1.5.1 (2014-08-06)

  - fixes broken MIDI handling in the DSSI plugin
  - fixes a crash when a folder is selected from 'import preset'


## 1.5.0 (2014-08-02)

  - new filter modes: notch and bypass
  - new filter controls for key-tracking and velocity sensitivity
  - OSC2 octave range increased
  - ring modulation can now be dialled in (instead of on/off)
  - LFO can now be routed to OSC1, OSC2, or both
  - fixes an audible click when using fast filter attack times
  - note and controller events are now rendered sample-accurate when
    run as a plugin, fixing MIDI timing jitter
  - DSSI & LV2 plug-in builds can now load any preset by
    right-clicking the UI background
  - bank & preset loading is now faster


## 1.4.2 (2014-04-16)

  - fixes use of alsa utilities from the 'Utils' menu
  - user specified jack client name now shown in window title
  - sounds can now be auditioned from the keyboard with ctrl-a
  - undo & redo now implemented
  - jack port auto connection can be stopped using a new option:
      amsynth --jack_autoconnect=false
  - more sound banks thanks to brian :)


## 1.4.1 (2014-02-03)

  - adds new preset banks: BriansBank12, 13 & 14
  - MIDI CCs for mapped parameters are now be sent in real-time
  - audition button note can now be changed
  - amsynth now responds to MIDI CC #10 panning messages
  - jack client name can now be set from the command line
  - fixes for issues with MIDI learn dialog


## 1.4.0 (2013-06-23)

  - LV2 support added
  - various improvements to sound engine, including:
    - mono & legato keyboard modes
    - portamento / glide control
    - extended OSC 2 detune range
    - switchable low-pass / high-pass / band-pass filter modes
    - switchable 12 / 24 db/Octave filter slopes
    - additional LFO waveforms (sawtooth up & down)
    - reduced aliasaing on oscillator's square / pulse waveform
  - now ships with substantial library of presets, thanks to brian@amsynth.com
  - preset banks are now stored in ~/.amsynth/banks
  - numerous other improvements and bug fixes


## 1.3.2 (2012-08-12)

  - fixes compilation error with GCC 4.7


## 1.3.1 (2012-06-23)

  - fixes check of external tools (e.g. vkeybd) - issue #34
  - fixes build on debian 6.0 - issue #35
  - fixes problem with sustain pedal release handling - issue #36
  - fixes DSSI 64-bit build - issue #22
  - fixes DSSI compatibility with Renoise
  - adds a basic DSSI GUI (no preset management yet)
  - default skin now installed as uncompressed files rather than a zip
  - some tidying up of code & build system


## 1.3.0 (2011-12-23)

  - adds a brand new skinned UI
  - added support for JACK-MIDI
  - added an icon
  - added support for alternate tunings
  - added MIDI-learn dialog for easier MIDI CC -> controller mapping
  - fixed denormal performance issue in reverb
  - fixed preset loading for some system locales
  - user interface tweaks
  - improvements to experimental DSSI build
  - #24 typo in quit confimation dialog
  - #26 setting midi channel from command line
  - #28 allow other MIDI drivers to be used alonside JACK-MIDI
  - fixed crash in DSSI plug-in build on 64-bit systems
  - fixed problem with auto-connection of JACK output ports


## 1.2.3 (2010-05-08)

  Bugs Fixed:

  - [2820935] hanging on exit
  - [2986164] build fails with GCC 4.5
  - [2988697] 'Virtual Keyboard' menu item doesn't gray out as expected
  - [2991732] reverb controls broken when built on certain GCC versions
  - [2993107] presets loaded incorrectly on some non-english systems

  Sound generation improvements:
  - Reduced amount of audible aliasing on saw tooth waveforms.
  - Improved performance of noise waveform generator.
  - Clamped maximum allowed pulsewidth on square / pulse waveform generator to prevent silence
  - Tweaked default for master_vol to a value that doesn't produce harmonic distortion

  Other Changes:
  - New factory preset - 'Dirty Pulsating Bass'
  - Added a preset modification indicator to title bar ('*')
  - Executable is no longer installed with suid enabled.
    for good low-latency performance please use the JACK audio system.
    Old behaviour can be re-enabled with ./configure --enable-realtime
  - Fixed compilation errors on various platforms caused by missing #include statements
  - Cleaned up compiler warnings
  - Other internal code improvements


## 1.2.2 (2009-07-12)

  Fixes various missing includes on some platforms / build configurations.
  Fixes compatibility issues with jackdmp.
  Better reporting of errors starting audio subsystem.
  Updated about dialog with list of contributors.


## 1.2.1 (2009-04-08)

  + fixed compilation errors on linux/gcc-4.3.2
  + fixed a buffer overrun crash (caught by fortify on ubuntu)


## 1.2.0 (2006-12-22)

  * nicer graphics
  * fixed crash upon startup on some machines
  * handle all notes off midi message
  * new about dialog
  * improved per-user installation process


## 1.1.0 (2005-10-26)

  + Updated GUI to use gtkmm2.x / gtk2.x
  + Fix build on latest GCC versions (4.0)
  + Stability improvements
  + Performance improvements


## 1.0.0 (2004-02-25)

  Initial stable release


## 1.0 beta 1 (2002-03-19)

  First public release
