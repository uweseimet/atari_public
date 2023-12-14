# Notes on the projects for Atari 16/32 bit computers in this repository

Each folder contains a separate project. There are no dependencies between these projects. They share a repository in order to reduce the maintenance overhead. There is a tag for each official release. For historic reasons the comments in some sources are in German.

- FIX24 - Limits address space to 24 bit. Work-arounds like this were required in the distant past to run some pieces of software for the ST also on the TT. Also see https://www.stcarchiv.de/stc1991/04/tt-manipulation-auf-24-bit (German only).

- MAGTOROM - Protects the MagiC operating system against write access. The binary is available on https://www.seimet.de/atari/en. Also see https://www.stcarchiv.de/stc1994/10/magtorom (German only).

- MODULES - Modules for the HDDRIVER driver, see https://www.hddriver.net/en/modules.html for details and binaries.

- NF_SCSI - SCSI Driver for Hatari and ARAnyM. See https://www.hddriver.net/en/scsidriver.html for details.

- OUTSIDE - The virtual memory manager. The binary is available on https://www.seimet.de/atari/en.

- ROMSPEED - Copies the TOS ROM to the Fast-RAM and accelerates the system. The binary is available on https://www.seimet.de/atari/en. Also see https://www.stcarchiv.de/stc1991/09/romspeed-und-virtuelle-speicherverwaltung (German only).

- RSSCONV - Converts .RSC and .H files created by a resource construction set like INTERFACE into a format suitable for assemblers like the EASYRIDER assembler.

- SCSI2PI - Atari client tools for the <a href="https://github.com/uweseimet/scsi2pi">SCSI2Pi</a>/<a href="https://github.com/PISCSI/piscsi">PiSCSI</a> projects. The binaries are available on https://www.hddriver.net/en/piscsi_tools.html.

- SCSI_MON - The SCSI Driver monitor. The binary is available on https://www.hddriver.net/en/scsidriver.html.

- SCTARGET - Sample code for the SCSI Driver target interface.

- SDRVTEST - The SCSI Driver/Firmware testsuite. The binaries are available on https://www.hddriver.net/en/scsidriver.html.

- XHDI_MON - The XHDI monitor. The binary is available on https://www.hddriver.net/en/downloads.html.
