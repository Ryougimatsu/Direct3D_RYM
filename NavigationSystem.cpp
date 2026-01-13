#include "NavigationSystem.h"
#include "map.h"
#include "Recast.h"
#include "DetourNavMeshBuilder.h"
#include "DetourCommon.h"
#include <vector>
#include <cmath>

using namespace DirectX;

// 单例静态实例
static NavigationSystem* g_Instance = nullptr;

// ==========================================
// 辅助结构：用于存储几何体数据
// ==========================================
struct InputGeom {
	std::vector<float> verts;
	std::vector<int> tris;

	void AddBox(const AABB& box) {
		float minX = box.min.x, minY = box.min.y, minZ = box.min.z;
		float maxX = box.max.x, maxY = box.max.y, maxZ = box.max.z;

		int startVert = (int)verts.size() / 3;

		float v[] = {
			minX, minY, minZ,  maxX, minY, minZ,  maxX, maxY, minZ,  minX, maxY, minZ,
			minX, minY, maxZ,  maxX, minY, maxZ,  maxX, maxY, maxZ,  minX, maxY, maxZ
		};
		for (float f : v) verts.push_back(f);

		int indices[] = {
			0,1,2, 0,2,3, // Front
			4,7,6, 4,6,5, // Back
			0,4,5, 0,5,1, // Bottom
			1,5,6, 1,6,2, // Right
			2,6,7, 2,7,3, // Top
			4,0,3, 4,3,7  // Left
		};
		for (int i = 0; i < 36; i += 3) {
			tris.push_back(startVert + indices[i]);
			tris.push_back(startVert + indices[i + 1]);
			tris.push_back(startVert + indices[i + 2]);

			tris.push_back(startVert + indices[i + 2]);
			tris.push_back(startVert + indices[i + 1]);
			tris.push_back(startVert + indices[i]);
		}
	}
};

// ==========================================
// 单例实现
// ==========================================
NavigationSystem* NavigationSystem::GetInstance() { return g_Instance; }

void NavigationSystem::Initialize() {
	if (!g_Instance) g_Instance = new NavigationSystem();
}

void NavigationSystem::Finalize() {
	if (g_Instance) { delete g_Instance; g_Instance = nullptr; }
}

NavigationSystem::NavigationSystem() {}

NavigationSystem::~NavigationSystem() { Cleanup(); }

void NavigationSystem::Cleanup() {
	dtFreeNavMeshQuery(m_navQuery);
	dtFreeNavMesh(m_navMesh);
	m_navQuery = nullptr;
	m_navMesh = nullptr;
}

// ==========================================
// 构建 NavMesh
// ==========================================
bool NavigationSystem::Build() {
	Cleanup();

	// --- 1. 准备几何体数据 ---
	InputGeom geom;
	const std::vector<MapObject>& objects = Map_GetObjects();

	if (objects.empty()) {
		return false;
	}

	for (const auto& obj : objects) {
		if (obj.KindId == MAP_KIND_WALL || obj.KindId == MAP_KIND_GROUND) {
			geom.AddBox(obj.Aabb);
		}
	}

	if (geom.verts.empty()) {
		return false;
	}

	// --- 2. 配置 Recast ---
	rcConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.cs = 0.3f;
	cfg.ch = 0.2f;
	rcCalcBounds(&geom.verts[0], geom.verts.size() / 3, cfg.bmin, cfg.bmax);

	// 扩大包围盒防止边缘裁剪
	cfg.bmin[0] -= 2.0f; cfg.bmin[1] -= 2.0f; cfg.bmin[2] -= 2.0f;
	cfg.bmax[0] += 2.0f; cfg.bmax[1] += 2.0f; cfg.bmax[2] += 2.0f;

	cfg.walkableHeight = (int)ceilf(1.8f / cfg.ch);
	cfg.walkableRadius = (int)ceilf(0.5f / cfg.cs);
	cfg.walkableClimb = (int)ceilf(0.5f / cfg.ch);
	cfg.walkableSlopeAngle = 45.0f;
	cfg.minRegionArea = (int)(8 * 8);
	cfg.mergeRegionArea = (int)(20 * 20);
	cfg.maxSimplificationError = 1.3f;
	cfg.maxEdgeLen = (int)(12.0f / cfg.cs);
	cfg.maxVertsPerPoly = 6;

	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	if (cfg.width == 0 || cfg.height == 0) {
		return false;
	}

	// --- 3. Recast 核心流程 ---
	rcContext ctx;
	rcHeightfield* hf = rcAllocHeightfield();
	if (!hf || !rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)) {
		return false;
	}

	unsigned char* trisAreas = new unsigned char[geom.tris.size() / 3];
	memset(trisAreas, 0, geom.tris.size() / 3);
	rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, &geom.verts[0], geom.verts.size() / 3, &geom.tris[0], geom.tris.size() / 3, trisAreas);
	if (!rcRasterizeTriangles(&ctx, &geom.verts[0], geom.verts.size() / 3, &geom.tris[0], trisAreas, geom.tris.size() / 3, *hf, cfg.walkableClimb)) {
		delete[] trisAreas; return false;
	}
	delete[] trisAreas;

	rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *hf);
	rcFilterLedgeSpans(&ctx, cfg.ch, cfg.walkableClimb, *hf);
	rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *hf);

	rcCompactHeightfield* chf = rcAllocCompactHeightfield();
	if (!chf || !rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf)) {
		rcFreeHeightField(hf);
		return false;
	}
	rcFreeHeightField(hf);

	if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf)) {
		rcFreeCompactHeightfield(chf);
		return false;
	}
	if (!rcBuildDistanceField(&ctx, *chf)) {
		rcFreeCompactHeightfield(chf);
		return false;
	}
	if (!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea)) {
		rcFreeCompactHeightfield(chf);
		return false;
	}

	rcContourSet* cset = rcAllocContourSet();
	if (!cset || !rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset)) {
		rcFreeCompactHeightfield(chf);
		return false;
	}

	rcPolyMesh* pmesh = rcAllocPolyMesh();
	if (!pmesh || !rcBuildPolyMesh(&ctx, *cset, cfg.maxVertsPerPoly, *pmesh)) {
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	if (pmesh->npolys == 0) {
		rcFreePolyMesh(pmesh);
		rcFreeContourSet(cset);
		rcFreeCompactHeightfield(chf);
		return false;
	}

	// 强制设置 Flags 为 1
	for (int i = 0; i < pmesh->npolys; ++i) {
		pmesh->flags[i] = 0x01;
	}

	rcFreeContourSet(cset);
	rcFreeCompactHeightfield(chf);

	// --- 4. Detour ---
	dtNavMeshCreateParams params;
	memset(&params, 0, sizeof(params));
	params.verts = pmesh->verts;
	params.vertCount = pmesh->nverts;
	params.polys = pmesh->polys;
	params.polyAreas = pmesh->areas;
	params.polyFlags = pmesh->flags;
	params.polyCount = pmesh->npolys;
	params.nvp = pmesh->nvp;

	params.walkableHeight = (float)cfg.walkableHeight * cfg.ch;
	params.walkableRadius = (float)cfg.walkableRadius * cfg.cs;
	params.walkableClimb = (float)cfg.walkableClimb * cfg.ch;
	rcVcopy(params.bmin, pmesh->bmin);
	rcVcopy(params.bmax, pmesh->bmax);
	params.cs = cfg.cs;
	params.ch = cfg.ch;
	params.buildBvTree = true;

	unsigned char* navData = 0;
	int navDataSize = 0;
	if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
		rcFreePolyMesh(pmesh);
		return false;
	}

	rcFreePolyMesh(pmesh);

	m_navMesh = dtAllocNavMesh();
	if (dtStatusFailed(m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA))) {
		dtFree(navData);
		return false;
	}

	m_navQuery = dtAllocNavMeshQuery();
	if (dtStatusFailed(m_navQuery->init(m_navMesh, 2048))) {
		return false;
	}

	m_filter.setIncludeFlags(0xFFFF);
	m_filter.setExcludeFlags(0);

	return true;
}

// ==========================================
// 寻找路径
// ==========================================
std::vector<XMFLOAT3> NavigationSystem::FindPath(XMFLOAT3 start, XMFLOAT3 end) {
	std::vector<XMFLOAT3> result;
	if (!m_navQuery) return result;

	// 吸附范围 X=10, Y=20, Z=10
	float extents[3] = { 10.0f, 20.0f, 10.0f };

	dtPolyRef startRef = 0;
	dtPolyRef endRef = 0;
	float startPt[3], endPt[3];
	float startPos[3] = { start.x, start.y, start.z };
	float endPos[3] = { end.x, end.y, end.z };

	dtStatus status;

	// 寻找最近的多边形 (Start)
	status = m_navQuery->findNearestPoly(startPos, extents, &m_filter, &startRef, startPt);
	if (dtStatusFailed(status) || startRef == 0) {
		return result;
	}

	// 寻找最近的多边形 (End)
	status = m_navQuery->findNearestPoly(endPos, extents, &m_filter, &endRef, endPt);
	if (dtStatusFailed(status) || endRef == 0) {
		return result;
	}

	// 计算路径
	dtPolyRef polys[256];
	int polyCount = 0;
	status = m_navQuery->findPath(startRef, endRef, startPt, endPt, &m_filter, polys, &polyCount, 256);

	if (dtStatusFailed(status) || polyCount == 0) {
		return result;
	}

	// 漏斗算法 (String Pulling)
	float straightPath[256 * 3];
	unsigned char straightPathFlags[256];
	dtPolyRef straightPathRefs[256];
	int straightPathCount = 0;

	m_navQuery->findStraightPath(startPt, endPt, polys, polyCount,
		straightPath, straightPathFlags, straightPathRefs, &straightPathCount, 256);

	for (int i = 0; i < straightPathCount; i++) {
		result.push_back({ straightPath[i * 3], straightPath[i * 3 + 1], straightPath[i * 3 + 2] });
	}

	return result;
}