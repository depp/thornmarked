# SDK

The Nintendo 64 SDK must be symlinked here in order to create builds for Nintendo 64.

Create a symlink named "ultra" that points to the "usr" directory inside a Nintendo 64 Developer OS/Library installation. It should be version 2.0L for Windows. The Irix version was made for the IRIX IDO compiler and will not work with GCC.

Create a symlink named "boot6012.bin" that points to the 6102 bootcode. A known good copy of this file has MD5 checksum e24dd796b2fa16511521139d28c8356b.

Also create a symlink to `pifdata.bin` for running CEN64.
