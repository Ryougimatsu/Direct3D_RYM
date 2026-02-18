#include "NavigationSystem.h"
#include "map.h"
#include "Recast.h"
#include "DetourNavMeshBuilder.h"
#include "DetourCommon.h"
#include <vector>
#include <cmath>

using namespace DirectX;

// 全局静态实例
static NavigationSystem* g_Instance = nullptr;

// ==========================================
// 辅助结构：用于存储输入给 Recast 的几何数据
// ==========================================
struct InputGeom {
	std::vector<float> verts; // 所有顶点坐标 (x, y, z, x, y, z...)
	std::vector<int> tris;    // 三角形索引 (v1, v2, v3...)

	// 将一个轴对齐包围盒 (AABB) 转换为三角形网格数据
	void AddBox(const AABB& box) {
		float minX = box.min.x, minY = box.min.y, minZ = box.min.z;
		float maxX = box.max.x, maxY = box.max.y, maxZ = box.max.z;

		// 当前顶点在数组中的起始索引
		int startVert = (int)verts.size() / 3;

		// 定义立方体的8个顶点
		float v[] = {
			minX, minY, minZ,  maxX, minY, minZ,  maxX, maxY, minZ,  minX, maxY, minZ,
			minX, minY, maxZ,  maxX, minY, maxZ,  maxX, maxY, maxZ,  minX, maxY, maxZ
		};
		for (float f : v) verts.push_back(f);

		// 定义立方体6个面的三角形索引（每个面2个三角形，共12个三角形）
		int indices[] = {
			0,1,2, 0,2,3, // Front (前)
			4,7,6, 4,6,5, // Back (后)
			0,4,5, 0,5,1, // Bottom (底)
			1,5,6, 1,6,2, // Right (右)
			2,6,7, 2,7,3, // Top (顶)
			4,0,3, 4,3,7  // Left (左)
		};
		for (int i = 0; i < 36; i += 3) {
			// 添加第一个三角形顶点
			tris.push_back(startVert + indices[i]);
			tris.push_back(startVert + indices[i + 1]);
			tris.push_back(startVert + indices[i + 2]);

			// 这里原代码似乎为了确保双面渲染或绕序问题，重复添加了一次反向/同样的三角形？
			// 通常标准的 Recast 输入只需要单面，但这里为了保险可能添加了双面。
			tris.push_back(startVert + indices[i + 2]);
			tris.push_back(startVert + indices[i + 1]);
			tris.push_back(startVert + indices[i]);
		}
	}
};

// ==========================================
// 单例管理与生命周期
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

// 清理 NavMesh 和查询对象
void NavigationSystem::Cleanup() {
	dtFreeNavMeshQuery(m_navQuery);
	dtFreeNavMesh(m_navMesh);
	m_navQuery = nullptr;
	m_navMesh = nullptr;
}

// ==========================================
// 核心功能：构建 NavMesh (Recast 流程)
// ==========================================
bool NavigationSystem::Build() {
	Cleanup(); // 构建前先清理旧数据

	// --- 1. 准备输入几何体 (Input Geometry) ---
	InputGeom geom;
	const std::vector<MapObject>& objects = Map_GetObjects(); // 获取游戏地图对象

	if (objects.empty()) {
		return false;
	}

	// 遍历所有地图对象，将墙壁和地面转换为几何数据
	for (const auto& obj : objects) {
		if (obj.KindId == MAP_KIND_WALL || obj.KindId == MAP_KIND_GROUND) {
			geom.AddBox(obj.Aabb);
		}
	}

	if (geom.verts.empty()) {
		return false;
	}

	// --- 2. 配置 Recast 生成参数 ---
	rcConfig cfg;
	memset(&cfg, 0, sizeof(cfg));
	cfg.cs = 0.3f; // Cell Size: 体素(Voxel)在 XZ 平面的大小 (单位: 米)
	cfg.ch = 0.2f; // Cell Height: 体素在 Y 轴的高度
	// 计算整个场景的包围盒
	rcCalcBounds(&geom.verts[0], geom.verts.size() / 3, cfg.bmin, cfg.bmax);

	// 扩大包围盒范围，防止物体刚好在边缘被裁剪掉
	cfg.bmin[0] -= 2.0f; cfg.bmin[1] -= 2.0f; cfg.bmin[2] -= 2.0f;
	cfg.bmax[0] += 2.0f; cfg.bmax[1] += 2.0f; cfg.bmax[2] += 2.0f;

	// 设置代理(Agent/角色)参数，用于计算可行走区域
	cfg.walkableHeight = (int)ceilf(1.8f / cfg.ch);   // 角色高度 (1.8m)
	cfg.walkableRadius = (int)ceilf(0.5f / cfg.cs);   // 角色半径 (0.5m)
	cfg.walkableClimb = (int)ceilf(0.5f / cfg.ch);    // 最大攀爬高度 (台阶高度 0.5m)
	cfg.walkableSlopeAngle = 45.0f;                   // 最大可通行坡度
	cfg.minRegionArea = (int)(8 * 8);                 // 过滤掉太小的孤立区域
	cfg.mergeRegionArea = (int)(20 * 20);             // 合并较小的区域
	cfg.maxSimplificationError = 1.3f;                // 轮廓简化误差
	cfg.maxEdgeLen = (int)(12.0f / cfg.cs);           // 轮廓边最大长度
	cfg.maxVertsPerPoly = 6;                          // 最终网格多边形的最大顶点数 (通常为6，即凸多边形)

	// 根据包围盒和体素大小计算 Grid 的宽高
	rcCalcGridSize(cfg.bmin, cfg.bmax, cfg.cs, &cfg.width, &cfg.height);

	if (cfg.width == 0 || cfg.height == 0) {
		return false;
	}

	// --- 3. Recast 光栅化与网格生成流程 ---
	rcContext ctx; // 上下文，用于记录日志或错误

	// 3.1 创建高度场 (Heightfield) - 体素化世界的基础
	rcHeightfield* hf = rcAllocHeightfield();
	if (!hf || !rcCreateHeightfield(&ctx, *hf, cfg.width, cfg.height, cfg.bmin, cfg.bmax, cfg.cs, cfg.ch)) {
		return false;
	}

	// 3.2 标记可行走的三角形
	unsigned char* trisAreas = new unsigned char[geom.tris.size() / 3];
	memset(trisAreas, 0, geom.tris.size() / 3);
	rcMarkWalkableTriangles(&ctx, cfg.walkableSlopeAngle, &geom.verts[0], geom.verts.size() / 3, &geom.tris[0], geom.tris.size() / 3, trisAreas);

	// 3.3 光栅化三角形到高度场 (Rasterization)
	if (!rcRasterizeTriangles(&ctx, &geom.verts[0], geom.verts.size() / 3, &geom.tris[0], trisAreas, geom.tris.size() / 3, *hf, cfg.walkableClimb)) {
		delete[] trisAreas; return false;
	}
	delete[] trisAreas;

	// 3.4 过滤高度场 (Filtering)
	rcFilterLowHangingWalkableObstacles(&ctx, cfg.walkableClimb, *hf); // 过滤低矮障碍物
	rcFilterLedgeSpans(&ctx, cfg.ch, cfg.walkableClimb, *hf);          // 过滤陡峭边缘
	rcFilterWalkableLowHeightSpans(&ctx, cfg.walkableHeight, *hf);     // 过滤高度不足以站立的空间

	// 3.5 构建紧凑高度场 (Compact Heightfield) - 优化内存并计算邻接信息
	rcCompactHeightfield* chf = rcAllocCompactHeightfield();
	if (!chf || !rcBuildCompactHeightfield(&ctx, cfg.walkableHeight, cfg.walkableClimb, *hf, *chf)) {
		rcFreeHeightField(hf);
		return false;
	}
	rcFreeHeightField(hf); // 不再需要原始高度场

	// 3.6 腐蚀可行走区域 (Erosion) - 根据角色半径向内收缩，防止贴墙穿模
	if (!rcErodeWalkableArea(&ctx, cfg.walkableRadius, *chf)) {
		rcFreeCompactHeightfield(chf);
		return false;
	}

	// 3.7 构建距离场和区域 (Regions) - 用于生成多边形
	if (!rcBuildDistanceField(&ctx, *chf)) {
		rcFreeCompactHeightfield(chf);
		return false;
	}
	if (!rcBuildRegions(&ctx, *chf, 0, cfg.minRegionArea, cfg.mergeRegionArea)) {
		rcFreeCompactHeightfield(chf);
		return false;
	}

	// 3.8 构建轮廓 (Contours) - 将体素边界转化为矢量线段
	rcContourSet* cset = rcAllocContourSet();
	if (!cset || !rcBuildContours(&ctx, *chf, cfg.maxSimplificationError, cfg.maxEdgeLen, *cset)) {
		rcFreeCompactHeightfield(chf);
		return false;
	}

	// 3.9 构建多边形网格 (Poly Mesh) - 最终的导航网格数据结构
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

	// 强制设置所有多边形的 Flag 为 1 (表示普通地面/可通行)
	// 这一步很重要，否则 Detour 寻路时会认为这些网格不可用
	for (int i = 0; i < pmesh->npolys; ++i) {
		pmesh->flags[i] = 0x01;
	}

	rcFreeContourSet(cset);
	rcFreeCompactHeightfield(chf);

	// --- 4. 初始化 Detour (运行时寻路库) ---
	dtNavMeshCreateParams params;
	memset(&params, 0, sizeof(params));
	params.verts = pmesh->verts;
	params.vertCount = pmesh->nverts;
	params.polys = pmesh->polys;
	params.polyAreas = pmesh->areas;
	params.polyFlags = pmesh->flags;
	params.polyCount = pmesh->npolys;
	params.nvp = pmesh->nvp;

	// 转换参数单位：从体素单位转回世界单位
	params.walkableHeight = (float)cfg.walkableHeight * cfg.ch;
	params.walkableRadius = (float)cfg.walkableRadius * cfg.cs;
	params.walkableClimb = (float)cfg.walkableClimb * cfg.ch;
	rcVcopy(params.bmin, pmesh->bmin);
	rcVcopy(params.bmax, pmesh->bmax);
	params.cs = cfg.cs;
	params.ch = cfg.ch;
	params.buildBvTree = true; // 构建 BVTree 加速空间查询

	unsigned char* navData = 0;
	int navDataSize = 0;

	// 创建 Detour 可用的 NavMesh 二进制数据
	if (!dtCreateNavMeshData(&params, &navData, &navDataSize)) {
		rcFreePolyMesh(pmesh);
		return false;
	}

	rcFreePolyMesh(pmesh);

	// 分配并初始化 dtNavMesh
	m_navMesh = dtAllocNavMesh();
	if (dtStatusFailed(m_navMesh->init(navData, navDataSize, DT_TILE_FREE_DATA))) {
		dtFree(navData);
		return false;
	}

	// 初始化 NavMesh 查询对象 (用于执行寻路算法)
	m_navQuery = dtAllocNavMeshQuery();
	if (dtStatusFailed(m_navQuery->init(m_navMesh, 2048))) { // 2048 是最大节点池大小
		return false;
	}

	// 设置默认过滤器 (允许所有 Flags)
	m_filter.setIncludeFlags(0xFFFF);
	m_filter.setExcludeFlags(0);

	return true;
}

// ==========================================
// 寻路逻辑
// ==========================================
std::vector<XMFLOAT3> NavigationSystem::FindPath(XMFLOAT3 start, XMFLOAT3 end) {
	std::vector<XMFLOAT3> result;
	if (!m_navQuery) return result;

	// 搜索范围：在目标点周围多大范围内寻找最近的 NavMesh 多边形
	// X=10, Y=20, Z=10 (Y通常大一些以适应高度差)
	float extents[3] = { 10.0f, 20.0f, 10.0f };

	dtPolyRef startRef = 0; // 起点所在的多边形引用 ID
	dtPolyRef endRef = 0;   // 终点所在的多边形引用 ID
	float startPt[3], endPt[3]; // 修正后的起点和终点（投影在 NavMesh 上的点）
	float startPos[3] = { start.x, start.y, start.z };
	float endPos[3] = { end.x, end.y, end.z };

	dtStatus status;

	// 1. 寻找离 startPos 最近的多边形 (Poly)
	status = m_navQuery->findNearestPoly(startPos, extents, &m_filter, &startRef, startPt);
	if (dtStatusFailed(status) || startRef == 0) {
		return result; // 找不到有效的起点多边形
	}

	// 2. 寻找离 endPos 最近的多边形
	status = m_navQuery->findNearestPoly(endPos, extents, &m_filter, &endRef, endPt);
	if (dtStatusFailed(status) || endRef == 0) {
		return result; // 找不到有效的终点多边形
	}

	// 3. 计算多边形路径 (粗略路径 - A* 算法)
	// 结果是一系列多边形的 ID
	dtPolyRef polys[256];
	int polyCount = 0;
	status = m_navQuery->findPath(startRef, endRef, startPt, endPt, &m_filter, polys, &polyCount, 256);

	if (dtStatusFailed(status) || polyCount == 0) {
		return result;
	}

	// 4. "拉绳"算法 (String Pulling) - 平滑路径
	// 将多边形序列转换为具体的直线坐标点序列
	float straightPath[256 * 3];
	unsigned char straightPathFlags[256]; // 路径点类型（起点、终点、拐点）
	dtPolyRef straightPathRefs[256];
	int straightPathCount = 0;

	m_navQuery->findStraightPath(startPt, endPt, polys, polyCount,
		straightPath, straightPathFlags, straightPathRefs, &straightPathCount, 256);

	// 5. 转换结果格式并返回
	for (int i = 0; i < straightPathCount; i++) {
		result.push_back({ straightPath[i * 3], straightPath[i * 3 + 1], straightPath[i * 3 + 2] });
	}

	return result;
}