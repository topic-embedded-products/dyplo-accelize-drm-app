#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>

#include <iostream>
#include <string>

#include "accelize/drm.h"
namespace drm = Accelize::DRM;

#define DRM_MEM_SIZE 0x10000

static void fpga_read_register(void *fpga_mem, uint32_t offset, uint32_t* value)
{
	*value = *(uint32_t*)((char*)fpga_mem + offset);
}

static void fpga_write_register(void *fpga_mem, uint32_t offset, uint32_t value)
{
	*(uint32_t*)((char*)fpga_mem + offset) = value;
}

static void *open_fpga_mmap()
{
	int fd = ::open("/dev/dyplo-drm", O_RDWR);
	if (fd == -1)
		return MAP_FAILED;

	void* map = ::mmap(NULL, DRM_MEM_SIZE /* size*/, PROT_READ | PROT_WRITE /* prot */, MAP_SHARED, fd, 0 /* offset */);
	::close(fd);

	return map;
}

int main(int argc, char **argv)
{
	void *fpga_mem = open_fpga_mmap();
	if (fpga_mem == MAP_FAILED) {
		std::cerr << "Failed to mmap /dev/dyplo-drm" << std::endl;
		return 2;
	}
	
	drm::DrmManager drm_manager(
	    // Configuration files paths
	    "conf.json",
	    "cred.json",

	    // Read/write register functions callbacks
	    [&](uint32_t offset, uint32_t* value) {
		fpga_read_register(fpga_mem, offset, value );
		return 0;
	    },

	    [&](uint32_t offset, uint32_t value) {
		fpga_write_register(fpga_mem, offset, value);
		return 0;
	    },

	    // Asynchronous error callback
	    [&](const std::string &err_msg) {
		std::cerr << err_msg << std::endl;
	    }
	);

	try {
	    drm_manager.activate();
	} catch( const drm::Exception& e ) {
	    std::cerr << "DRM error: " << e.what() << std::endl;
	    return 1;
	}

	return 0;
}
