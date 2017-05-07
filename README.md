# powerapplet_tray

This is a modified version of 
[powerapplet_tray 2.6](http://distro.ibiblio.org/puppylinux/sources/p/powerapplet_tray-2.6.tar.bz2)
that includes /sys/class/power_supply as a source 
for battery stats in addition to /proc/acpi, and /proc/apm.

This is just a make-it-work-dammit, so don't expect this to be
100% stable.

I would base this on powerapplet_tray 2.7.2, but I can't find the 
source for the life of me! :P

This was designed for my MacBook (13", Mid-2009) on
Slacko64-6.3.2, which didn't properly report ACPI stats. 
As a result, there was no battery icon in the tray. 

I have also rearranged some code, added a few error checks,
and added comments, including one of the most complicated Bash
one-liners in history. 

## Compiling and building

If you are compiling this on Puppy, you need the devx libs.

#### Compiling

```
cd /path/to/powerapplet_tray
./compile
cp *.svg /usr/share/pixmaps/puppy/
```

#### Testing

```
./powerapplet_tray
```

#### Installing

```
cp powerapplet_tray /usr/bin/

```

### License

```
Copyright (C) 2010 Barry Kauler, bkhome.org

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
```
