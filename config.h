/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if your system is a PowerPC. */
/* #undef ARCH_POWERPC */

/* Config dir to use */
#define BMP_RCPATH ".audacious"

/* Path to OSS DSP, really just a data pipe, default is /dev/dsp. */
#define DEV_DSP "/dev/dsp"

/* Path to OSS mixer, default is /dev/mixer. */
#define DEV_MIXER "/dev/mixer"

/* Define to disable per user plugin directory */
/* #undef DISABLE_USER_PLUGIN_DIR */

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define ENABLE_NLS 1

/* Define if FLAC output part should be built */
#define FILEWRITER_FLAC 1

/* Define if MP3 output part should be built */
/* #undef FILEWRITER_MP3 */

/* Define if Vorbis output part should be built */
#define FILEWRITER_VORBIS 1

/* Define to 1 if your system has AltiVec. */
/* #undef HAVE_ALTIVEC */

/* Define to 1 if your system has an altivec.h file. */
/* #undef HAVE_ALTIVEC_H */

/* Define to 1 if you have the <Carbon/Carbon.h> header file. */
/* #undef HAVE_CARBON_CARBON_H */

/* Define to 1 if you have the <CoreServices/CoreServices.h> header file. */
/* #undef HAVE_CORESERVICES_CORESERVICES_H */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define to 1 if you have the <fnmatch.h> header file. */
#define HAVE_FNMATCH_H 1

/* Define to 1 if you have the <fts.h> header file. */
#define HAVE_FTS_H 1

/* Define to 1 if you have the `getmntinfo' function. */
/* #undef HAVE_GETMNTINFO */

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define if you have HardSID for libSIDPlay 2 */
/* #undef HAVE_HARDSID_BUILDER */

/* Define if you have the iconv() function. */
/* #undef HAVE_ICONV */

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <jack/jack.h> header file. */
#define HAVE_JACK_JACK_H 1

/* Define to 1 if you have the <limits.h> header file. */
#define HAVE_LIMITS_H 1

/* Define to 1 if you have the <linux/joystick.h> header file. */
#define HAVE_LINUX_JOYSTICK_H 1

/* Define to 1 if you have the `lrintf' function. */
#define HAVE_LRINTF 1

/* Define to 1 if you have the <machine/soundcard.h> header file. */
/* #undef HAVE_MACHINE_SOUNDCARD_H */

/* Define to 1 if you have the <memory.h> header file. */
#define HAVE_MEMORY_H 1

/* Define to 1 if you have the `mkdtemp' function. */
#define HAVE_MKDTEMP 1

/* Define to 1 if you have the <mpcdec/config_types.h> header file. */
#define HAVE_MPCDEC_CONFIG_TYPES_H 1

/* Define if you have the FreeBSD newpcm driver */
/* #undef HAVE_NEWPCM */

/* Define if the OSS output plugin should be built */
#define HAVE_OSS 1

/* Define if the OSS4 output plugin should be built */
/* #undef HAVE_OSS4 */

/* Define if you have reSID for libSIDPlay 2 */
#define HAVE_RESID_BUILDER 1

/* Set to 1 if you have libsamplerate. */
#define HAVE_SAMPLERATE 1

/* Define if you have and want to use libSIDPlay1 */
/* #undef HAVE_SIDPLAY1 */

/* Define if you have and want to use libSIDPlay2 */
#define HAVE_SIDPLAY2 1

/* Define to 1 if you have the <soundcard.h> header file. */
/* #undef HAVE_SOUNDCARD_H */

/* Define to 1 if your system has SSE2 */
#define HAVE_SSE2 1

/* Define to 1 if you have the `statvfs' function. */
#define HAVE_STATVFS 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the `strtoul' function. */
#define HAVE_STRTOUL 1

/* Define to 1 if you have the <sys/ioctl.h> header file. */
#define HAVE_SYS_IOCTL_H 1

/* Define to 1 if you have the <sys/soundcard.h> header file. */
#define HAVE_SYS_SOUNDCARD_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if you have the <wchar.h> header file. */
#define HAVE_WCHAR_H 1

/* X Composite extension available */
#define HAVE_XCOMPOSITE 

/* Name of package */
#define PACKAGE "audacious-plugins"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "bugs+audacious-plugins@atheme.org"

/* Define to the full name of this package. */
#define PACKAGE_NAME "audacious-plugins"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "audacious-plugins 1.5.1"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "audacious-plugins"

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.5.1"

/* Define the shared module suffix extension on your platform. */
#define SHARED_SUFFIX ".so"

/* Define to 1 if you have the ANSI C header files. */
#define STDC_HEADERS 1

/* Define to symbol prefix, if any */
/* #undef SYMBOL_PREFIX */

/* Define if character set detection enabled */
/* #undef USE_CHARDET */

/* Define if D-Bus support enabled */
#define USE_DBUS 1

/* Define if building with IPv6 support */
/* #undef USE_IPV6 */

/* Version number of package */
#define VERSION "1.5.1"

/* Define to 1 if your processor stores words with the most significant byte
   first (like Motorola and SPARC, unlike Intel and VAX). */
/* #undef WORDS_BIGENDIAN */
