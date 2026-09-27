# akaifat
C++ library to read/write Akai FAT12/FAT16 volumes with 16.3 file names (MPC60/MPC2000/MPC2000XL)

FAT allocation encoding and Akai filename metadata are separate: the FAT12 codec
uses standard packed 12-bit entries; `AkaiFatFileSystem` explicitly interprets
Akai directory names (extra characters at bytes 12–19). This is not a generic
DOS/VFAT filesystem API. The existing FAT16 BPB/root-directory classes also serve
FAT12 because their layouts are shared; their public names remain compatible.

Reading recognizes FAT12 by data-cluster count, retaining support for explicitly
labelled small FAT16 volumes. Unsigned boot sectors are accepted only for the
observed complete 800 KiB MPC60 profile (512-byte sectors, ten sectors per track,
two heads, two sectors per cluster, 112 root entries, two three-sector FATs).
Opening or writing does not add a PC boot signature or change the geometry.
The formatter still creates FAT16 volumes; FAT12 formatting is not added.
