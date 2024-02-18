#include "memory_mapped_file.hpp"

#include <cassert>

#if defined(PLATFORM__ANDROID) && defined(VKB_ANDROID_ASSET_FILESYSTEM)
#	include <android/asset_manager.h>
#elif defined(PLATFORM__WINDOWS)
#	include <Windows.h>
#else
#	include <sys/mman.h>
#	include <sys/stat.h>
#endif

namespace vkb
{
namespace filesystem
{

struct MemoryMappedFile::PlatformData
{
#if defined(PLATFORM__ANDROID) && defined(VKB_ANDROID_ASSET_FILESYSTEM)
	// Windows file handle and mapping handle
	AAsset *asset{nullptr};
#elif defined(PLATFORM__WINDOWS)
	HANDLE file{INVALID_HANDLE_VALUE};
	HANDLE mapping{INVALID_HANDLE_VALUE};
#else
	// Unix file descriptor
	int   file_descriptor{-1};
#endif
};

MemoryMappedFile::MemoryMappedFile(const Path &path) :
    platform_data_ptr(std::make_unique<PlatformData>())
{
#if defined(PLATFORM__ANDROID) && defined(VKB_ANDROID_ASSET_FILESYSTEM)
	auto &asset = platform_data_ptr->asset;

	asset = AAssetManager_open(getAssetManager(), filename.c_str(), AASSET_MODE_BUFFER);
	assert(asset);
	bytes  = AAsset_getLength(_asset);
	mapped = reinterpret_cast<const uint8_t *>(AAsset_getBuffer(_asset));
#elif defined(PLATFORM__WINDOWS)
	auto  &file    = platform_data_ptr->file;
	auto  &mapping = platform_data_ptr->mapping;

	file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (file == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to open file");
	}
	{
		DWORD file_size_high;
		bytes = GetFileSize(file, &file_size_high);
		bytes += (((size_t) file_size_high) << 32);
	}
	mapping = CreateFileMapping(file, NULL, PAGE_READONLY, 0, 0, NULL);
	if (0 == mapping || mapping == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to create mapping");
	}
	mapped        = reinterpret_cast<const uint8_t *>(MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, 0));
#else
	auto &file = platform_data_ptr->file_descriptor;

	file = open(filename.c_str(), O_RDONLY);
	if (-1 == file)
	{
		throw std::runtime_error("Failed to open file");
	}

	{
		stat stat_info;
		if (-1 == fstat(file, &stat_info))
		{
			throw std::runtime_error("Unable to stat file");
		}
		bytes = stat_info.st_size;
	}

	mapped = reinterpret_cast<const uint8_t *>(mmap(nullptr, size, PROT_READ, MAP_PRIVATE, file, 0));
	if (mapped == MAP_FAILED)
	{
		throw std::runtime_error("Unable to mmap file");
	}
#endif
	assert(bytes > 0);
	assert(mapped);
}

MemoryMappedFile::~MemoryMappedFile()
{
#if defined(PLATFORM__ANDROID) && defined(VKB_ANDROID_ASSET_FILESYSTEM)
	auto &asset = platform_data_ptr->asset;
	AAsset_close(_asset);
#elif defined(PLATFORM__WINDOWS)
	auto &file    = platform_data_ptr->file;
	auto &mapping = platform_data_ptr->mapping;

	UnmapViewOfFile(mapped);
	CloseHandle(mapping);
	CloseHandle(file);
#else
	auto &file = platform_data_ptr->file_descriptor;
	munmap(mapped, bytes);
	close(file);
#endif
}

const uint8_t *MemoryMappedFile::data() const
{
	return mapped;
}

size_t MemoryMappedFile::size() const
{
	return bytes;
}

}        // namespace filesystem
}        // namespace vkb
