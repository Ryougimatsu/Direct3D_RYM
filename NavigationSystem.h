#pragma once
#include <vector>
#include <DirectXMath.h>

// Recast & Detour 的头文件 (请确保你下载并配置好了库路径)
#include "Recast.h"
#include "DetourNavMesh.h"
#include "DetourNavMeshQuery.h"

class NavigationSystem
{
public:
	static NavigationSystem* GetInstance();
	static void Initialize();
	static void Finalize();
	bool Build();
	std::vector<DirectX::XMFLOAT3> FindPath(DirectX::XMFLOAT3 startPos, DirectX::XMFLOAT3 endPos);

private:
	NavigationSystem();
	~NavigationSystem();

	void Cleanup();

	dtNavMesh* m_navMesh = nullptr;
	dtNavMeshQuery* m_navQuery = nullptr;

	dtQueryFilter m_filter;
};