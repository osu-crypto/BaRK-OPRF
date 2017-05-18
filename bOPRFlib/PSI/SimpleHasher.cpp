#include "SimpleHasher.h"
#include "Crypto/sha1.h"
#include "Crypto/PRNG.h"
#include <random>
#include "Common/Log.h"
#include <numeric>
#include <algorithm>

namespace bOPRF
{


	SimpleHasher::SimpleHasher()
	{
	}


	SimpleHasher::~SimpleHasher()
	{
	}

	void SimpleHasher::print() const
	{

		Log::out << Log::lock;
		/*for(u64 j=0;j<3;j++)
			Log::out << "  mHashSeeds[" << j << "] " << mHashSeeds[j] << Log::endl;*/
		Log::out << "Simple Hasher  " << Log::endl;
		for (u64 i = 0; i < mBins.size(); ++i)
			//	for (u64 i = 0; i <1; ++i)
		{
			Log::out << "Bin #" << i << Log::endl;

			Log::out << " contains " << mBinSizes[i] << " elements" << Log::endl;

			for (u64 j = 0; j < mBinSizes[i]; ++j)
			{
				Log::out << "    idx=" << mBins(i,j).mIdx << "  hIdx=" << mBins(i,j).mHashIdx << Log::endl;
				//	Log::out << "    " << mBins[i].first[j] << "  " << mBins[i].second[j] << Log::endl;

			}

			Log::out << Log::endl;
		}

		Log::out << Log::endl;
		Log::out << Log::unlock;
	}

	void SimpleHasher::init(u64 cuckooSize, u64 simpleSize, u64 statSecParam, u64 numHashFunction)
	{		
		if (numHashFunction != 3) throw std::runtime_error(LOCATION);

		mSimpleSize = simpleSize;
		mCuckooSize = cuckooSize;
		mBinCount = 1.2*cuckooSize;
		mMaxBinSize = get_bin_size(mBinCount, simpleSize * numHashFunction, statSecParam);
		mBins.resize(mBinCount, mMaxBinSize);
		mBinSizes.resize(mBinCount, 0);
	}


	u64 SimpleHasher::insertItems(std::array<std::vector<block>, 4> hashs)
	{


		u64 cnt=0;
		u8 xrHash[SHA1::HashSize];

		SHA1 f;

		for (u64 i = 0; i < hashs[0].size(); ++i)
		{
			u64 addr;
			std::array<u64, 3> idxs{ -1,-1,-1 };

			for (u64 j = 0; j < 3; ++j) {

				u64 xrHashVal = *(u64*)&hashs[j][i] % mBinCount;

				addr = xrHashVal % mBinCount;

				{
					auto bb = mBinSizes[addr]++;

					mBins(addr, bb).mIdx  =i;
					mBins(addr, bb).mHashIdx=j;
					cnt++;
				}
			}
		}	
		return cnt;
	}

}
