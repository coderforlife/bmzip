// bmzip: program for compressing and decompressing the bootmgr file
// Copyright (C) 2012  Jeffrey Bush  jeff@coderforlife.com
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#ifndef GET_BOOTMANAGER_VERSION_H
#define GET_BOOTMANAGER_VERSION_H

struct Version {
	Version();
	unsigned short Minor, Major, Revision, Build;
};

const Version GetBootmanagerVersion(const void* data);

#endif