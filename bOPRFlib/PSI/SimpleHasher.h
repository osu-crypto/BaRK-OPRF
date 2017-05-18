#pragma once
#include "Common/Defines.h"
#include "Common/BitVector.h"
#include "Common/ArrayView.h"
#include "Common/Matrix.h"

namespace bOPRF
{
	
	class SimpleHasher
	{
	public:
		SimpleHasher();
		~SimpleHasher();

		struct item
		{
			u64 mIdx;
			u64 mHashIdx;
		};

		//struct Bin
		//{

		//	Bin() :mSize(0) {}


		//	//Matrix<item> mItems;
		//	//std::array<item, 21> mItems;
		//	u64 mSize;

		//};


		u64 mBinCount, mMaxBinSize, mSimpleSize, mCuckooSize;
		Matrix<item> mBins;
		std::vector<u64> mBinSizes;
		block mHashSeed;
		void print() const;

	
		void init(u64 cuckooSize, u64 simpleSize, u64 statSecParam = 40, u64 numHashFunction = 3);
		u64 insertItems(std::array<std::vector<block>,4> hashs);

	};

}
