#pragma once

#include "core/macro.h"

#include <string>

namespace tt
{

class Filesystem
{
public:
	void SetAssetBaseDir(const std::string& dir) { m_asset_base_dir = dir; }
	auto& GetAssetBaseDir() const { return m_asset_base_dir; }

	static bool IsExists(const char* filepath);

private:
	std::string m_asset_base_dir;

	TT_SINGLETON_DECLARATION(Filesystem)

}; // Filesystem

}