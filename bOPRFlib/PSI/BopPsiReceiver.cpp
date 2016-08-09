#include "BopPsiReceiver.h"
#include <future>
#include "Crypto/PRNG.h"
#include "Crypto/Commit.h"
#include "PSI/SimpleHasher.h"
#include "Common/Log.h"
#include "OT/Base/naor-pinkas.h"
#include <unordered_map>


namespace bOPRF
{
	std::string hexString(u8* data, u64 length)
	{
		std::stringstream ss;

		for (u64 i = 0; i < length; ++i)
		{

			ss << std::hex << std::setw(2) << std::setfill('0') << (u16)data[i];
		}

		return ss.str();
	}

	BopPsiReceiver::BopPsiReceiver()
	{
	}


	BopPsiReceiver::~BopPsiReceiver()
	{
	}

	void BopPsiReceiver::init(u64 n, u64 statSecParam, Channel & chl0, SSOtExtReceiver& ots, block seed)
	{
		init(n, statSecParam, { &chl0 }, ots, seed);
	}


	void BopPsiReceiver::init(u64 n, u64 statSecParam, const std::vector<Channel*>& chls, SSOtExtReceiver& otRecv, block seed)
	{

		mStatSecParam = statSecParam;
		mN = n;

		mNumStash = get_stash_size(n);

		gTimer.setTimePoint("Init.start");
		PRNG prngHashing(seed);
		block myHashSeeds;
		myHashSeeds = prngHashing.get_block();
		auto& chl0 = *chls[0];


		// we need a random hash function, so both commit to a seed and then decommit later
		chl0.asyncSend(&myHashSeeds, sizeof(block));
		block theirHashingSeeds;
		chl0.asyncRecv(&theirHashingSeeds, sizeof(block));

		//gTimer.setTimePoint("Init.hashSeed");


		// init Cuckoo hash
		mBins.init(n);

		// makes codeword for each bins
		mSSOtMessages.resize(mBins.mBinCount + mNumStash);

		//do base OT
			if (otRecv.hasBaseSSOts() == false)
			{
				//Timer timer;
				gTimer.setTimePoint("Init: BaseSSOT start");
				Log::setThreadName("receiver");
				BaseSSOT baseSSOTs(chl0, OTRole::Sender);
				baseSSOTs.exec_base(prngHashing);
				baseSSOTs.check();
				otRecv.setBaseSSOts(baseSSOTs.sender_inputs);
				gTimer.setTimePoint("Init: BaseSSOT done");
				//	Log::out << gTimer;
			}

		mHashingSeed = myHashSeeds ^ theirHashingSeeds;

		//gTimer.setTimePoint("Init.ExtStart");
		//extend OT
		otRecv.Extend(mBins.mBinCount + mNumStash, mSSOtMessages, chl0);

		//gTimer.setTimePoint("r Init.Done");
		//	Log::out << gTimer;
	}

	void BopPsiReceiver::sendInput(std::vector<block>& inputs, Channel & chl)
	{
		sendInput(inputs, { &chl });
	}
	void BopPsiReceiver::sendInput(std::vector<block>& inputs, const std::vector<Channel*>& chls)
	{
		//online phase.

		// check that the number of inputs is as expected.
		if (inputs.size() != mN)
			throw std::runtime_error("inputs.size() != mN");
		gTimer.setTimePoint("Online.Start");

		//asign channel
		auto& chl = *chls[0];

		//hash all items, use for: 1) arrage each item to bin using Cuckoo; 
		//                         2) use for psedo-codeword.
		std::array<AES, 4> AESHASH;
		TODO("make real keys seeds");
		for (u64 i = 0; i < AESHASH.size(); ++i)
			AESHASH[i].setKey(_mm_set1_epi64x(i));

		std::array<std::vector<block>, 4> aesHashBuffs;
		aesHashBuffs[0].resize(inputs.size());
		aesHashBuffs[1].resize(inputs.size());
		aesHashBuffs[2].resize(inputs.size());
		aesHashBuffs[3].resize(inputs.size());

		for (u64 i = 0; i < inputs.size(); i += stepSize)
		{
			auto currentStepSize = std::min(stepSize, inputs.size() - i);

			AESHASH[0].ecbEncBlocks(inputs.data() + i, currentStepSize, aesHashBuffs[0].data() + i);
			AESHASH[1].ecbEncBlocks(inputs.data() + i, currentStepSize, aesHashBuffs[1].data() + i);
			AESHASH[2].ecbEncBlocks(inputs.data() + i, currentStepSize, aesHashBuffs[2].data() + i);
			AESHASH[3].ecbEncBlocks(inputs.data() + i, currentStepSize, aesHashBuffs[3].data() + i);
		};

		//insert item to corresponding bin
		mBins.insertItems(aesHashBuffs); 
		//mBins.print();


		SHA1 sha1;
		u8 hashBuff1[SHA1::HashSize];
		
		//random seed
		PRNG prngRandome(_mm_set_epi32(42534612345, 34557734565, 211234435, 23987045));

		auto binStart = 0;
		auto binEnd = mBins.mBinCount;

		u64 maskSize = get_mask_size(inputs.size()); //by byte
		u64 codeWordSize = get_codeword_size(inputs.size()); //by byte

		TODO("define type for first key of localMasks (u32 or u64)")
		//NOTE: you need to change the type of localMasks to
			//std::unordered_map<u64, std::pair<block, u64>> localMasks;
			//if # input > 2^20
			//ALSO: need to change all reference!
		std::unordered_map<u64, std::pair<block, u64>> localMasks;
		localMasks.reserve((mBins.mBinCount + mNumStash));

		blockBop codeWord;
		// for each bin, compute our masks, and then the corresponding PSI mask.
		TODO("run in parallel");

		for (u64 stepIdx = binStart; stepIdx < binEnd; stepIdx += stepSize)
		{
			// compute the size of current step & end index.
			auto currentStepSize = std::min(stepSize, binEnd - stepIdx);
			auto stepEnd = stepIdx + currentStepSize;

			// make a buffer for the pseudo-code we need to send
			std::unique_ptr<ByteStream> buff(new ByteStream());
			buff->resize((sizeof(blockBop)*currentStepSize));
			auto myOt = buff->getArrayView<blockBop>();

			// for each bin, do encoding
			for (u64 bIdx = stepIdx, i = 0; bIdx < stepEnd; bIdx++, ++i)
			{
				auto& item = mBins.mBins[bIdx];
				block mask(ZeroBlock);

				if (item.isEmpty() == false)
				{			
					codeWord.elem[0] = aesHashBuffs[0][item.mIdx];
					codeWord.elem[1] = aesHashBuffs[1][item.mIdx];
					codeWord.elem[2] = aesHashBuffs[2][item.mIdx];
					codeWord.elem[3] = aesHashBuffs[3][item.mIdx];

					// encoding will send to the sender.
					myOt[i] =
						codeWord
						^ mSSOtMessages[bIdx][0]
						^ mSSOtMessages[bIdx][1];

				
					//compute my mask
					sha1.Reset();
					sha1.Update((u8*)&bIdx, sizeof(u64));
					sha1.Update((u8*)&mSSOtMessages[bIdx][0], codeWordSize);
					sha1.Final(hashBuff1);


					// store the my mask value here					
					memcpy(&mask, hashBuff1, maskSize); 


				//if (bIdx < 1)
				//	{
				//		//Log::out << "r  myMasksIter[" << bIdx << "]:" << hexString(myMasksIter, maskSize) << Log::endl;
				//		//Log::out << "r  mask[" << bIdx << "]:" << mask << Log::endl;
				//	//	Log::out << "r  mask[" << bIdx << "]:" << hexString((u8*)&mask, sizeof(u64)) << Log::endl;
				//	}	
				//	u64 aa = *(u64*)&mask;
					//	Log::out << "r  aa" << hexString((u8*)&aa, sizeof(u64)) << Log::endl;


					//NOTE: if the key of localMask have type u64, change
					//localMasks.emplace(*(u64*)&mask, std::pair<block, u64>(mask, item.mIdx));
					localMasks.emplace(*(u64*)&mask, std::pair<block, u64>(mask, item.mIdx));

				}
				else
				{
					// no item for this bin, just use a dummy.
					myOt[i] = prngRandome.get_block512(codeWordSize);
				}				
			}
			// send the masks for the current step
			chl.asyncSend(std::move(buff));
		}// Done with compute the masks for the main set of bins. 

		// buffer for the stash.
		std::unique_ptr<ByteStream> buff(new ByteStream());
		buff->resize((sizeof(blockBop)*mBins.mStash.size()));
		auto myOt = buff->getArrayView<blockBop>();


		// compute the encoding for each item in the stash.
		for (u64 i = 0, otIdx = mBins.mBinCount; i < mBins.mStash.size(); ++i, ++otIdx)
		{
			auto& item = mBins.mStash[i];
			block mask(ZeroBlock);

			if (item.isEmpty() == false)
			{
				codeWord.elem[0] = aesHashBuffs[0][item.mIdx];
				codeWord.elem[1] = aesHashBuffs[1][item.mIdx];
				codeWord.elem[2] = aesHashBuffs[2][item.mIdx];
				codeWord.elem[3] = aesHashBuffs[3][item.mIdx];

				myOt[i] =
					codeWord
					^ mSSOtMessages[i][0]
					^ mSSOtMessages[i][1];

				sha1.Reset();
				sha1.Update((u8*)&otIdx, sizeof(u64));
				sha1.Update((u8*)&mSSOtMessages[otIdx][0], codeWordSize);
				sha1.Final(hashBuff1);
				memcpy(&mask, hashBuff1, maskSize); 

				//NOTE: if the key of localMask have type u64, change
				//localMasks.emplace(*(u64*)&mask, std::pair<block, u64>(mask, item.mIdx));
				localMasks.emplace(*(u64*)&mask, std::pair<block, u64>(mask, item.mIdx));
			}
			else
			{
				myOt[i] = prngRandome.get_block512(codeWordSize);
			}
		}


		chl.asyncSend(std::move(buff));

		ByteStream recvBuff;

		// receive the PSI values.
		chl.recv(recvBuff);

		// double check the size.
		u64 cntMask = (3 + mNumStash)*mBins.mN;
		auto expectedSize = (cntMask* maskSize);

		gTimer.setTimePoint("Online.MaskReceived");
		if (recvBuff.size() != expectedSize)
		{
			Log::out << "recvBuff.size() != expectedSize" << Log::endl;
			throw std::runtime_error("rt error at " LOCATION);
		}

		auto theirMasks = recvBuff.data();

		for (u64 i = 0; i < cntMask; ++i)
		{
			//NOTE: if the key of localMask have type u64, change to
			//auto& kk = *(u64*)(theirMasks);

			auto& kk = *(u64*)(theirMasks);

			// check 32 first bits
			auto match = localMasks.find(kk);

			//if match, check for whole bits
			if (match != localMasks.end())
			{
				if (memcmp(theirMasks, &match->second.first, maskSize) == 0) // check full mask
				{
					mIntersection.push_back(match->second.second);
					//	Log::out << "#id: " << match->second.second << Log::endl;
				}
			}
			theirMasks += maskSize;

		}
		gTimer.setTimePoint("Online.Done"); 
	//	Log::out << gTimer << Log::endl;
	}
}