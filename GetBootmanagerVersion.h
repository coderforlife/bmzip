#ifndef GET_BOOTMANAGER_VERSION_H
#define GET_BOOTMANAGER_VERSION_H

struct Version {
	Version();
	unsigned short Minor, Major, Revision, Build;
};

const Version GetBootmanagerVersion(const void* data);

#endif