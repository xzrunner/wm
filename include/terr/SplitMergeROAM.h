#pragma once

#include <functional>

#include <assert.h>

#define DEFAULT_POLYGON_TARGET	10000

namespace terr
{

struct BinTriPool;

class SplitMergeROAM
{
public:
	struct Pos
	{
		int16_t x, y;

		Pos() : x(0), y(0) {}
		Pos(int16_t x, int16_t y) : x(x), y(y) {}

		void Assign(int16_t x, int16_t y) {
			this->x = x; this->y = y;
		}
	};

	struct BinTri
	{
		BinTri* left_child = nullptr;
		BinTri* right_child = nullptr;
		BinTri* base_neighbor = nullptr;
		BinTri* left_neighbor = nullptr;
		BinTri* right_neighbor = nullptr;

		BinTri* parent = nullptr;

		// for freelist
		BinTri* next = nullptr;

		Pos v0, v1, va;
		uint8_t level = 0;
//		uint8_t variance = 0;
		uint8_t padding;
		uint32_t number = 0;

		uint8_t flags = 0;
		uint8_t padding1[3];
	};

	struct BinTriPool
	{
	public:
		BinTriPool(int cap);
		~BinTriPool();

		BinTri* Alloc();
		void Free(BinTri* tri);

		int GetSize() const { return m_size; }
		int GetNext() const { return m_next; }

		void Reset();

	private:
		BinTri* m_tris;
		int m_size, m_next;

		BinTri* m_freelist;

	}; // BinTriPool

public:
	SplitMergeROAM(int size, BinTriPool& pool);
	~SplitMergeROAM();

	void Init();
	bool Update();
	void Draw() const;

public:
	struct CallbackFuncs
	{
		std::function<uint8_t(int x, int y)> get_height = nullptr;
		std::function<float(int x, int y)> dis_to_camera = nullptr;
		std::function<bool(float x, float y, float radius)> sphere_in_frustum = nullptr;
		std::function<void(int x, int y)> send_vertex = nullptr;
	}; // CallbackFuncs

	void RegisterCallback(const CallbackFuncs& cb) { m_cb = cb; }

private:
	void Reset();

	// init
	void ComputeVariances();
	uint8_t ComputeTriangleVariance(int num, const Pos& v0, const Pos& v1, const Pos& va, int level);

	// update
	void AdjustQualityConstant();

	void RecurseTesselate(BinTri* tri, bool entirely_in_frustum);

	void Split(BinTri* tri);
	void SplitNoBaseN(BinTri* tri);

	bool GoodForMerge(BinTri* tri) const;
	void Merge(BinTri* tri);
	void MergeNoBaseN(BinTri* tri);

	// draw
	void RenderTri(BinTri* tri, const Pos& v0, const Pos& v1, const Pos& va) const;

private:
	CallbackFuncs m_cb;

	int m_size;

	BinTriPool& m_pool;

	BinTri* m_nw_tri = nullptr;
	BinTri* m_se_tri = nullptr;

	int m_levels;

	float m_quality_constant;

	int m_used_nodes;		// number of nodes needed for variance tree
	uint8_t* m_variance;	// the variance implicit binary tree

	float* m_hypo_len;		// table of hypotenuse lengths

	int m_split_cutoff;

	//// stat
	//int m_drawn_tris;

}; // SplitMergeROAM

}