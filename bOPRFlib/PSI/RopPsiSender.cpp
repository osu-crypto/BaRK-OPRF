#include "BopPsiSender.h"
#include "Crypto/Commit.h"
#include "Common/Log.h"
#include "Common/Timer.h"
#include "OT/Base/naor-pinkas.h"

namespace bOPRF
{

	BopPsiSender::BopPsiSender()
	{
	}

	BopPsiSender::~BopPsiSender()
	{
	}
	extern std::string hexString(u8* data, u64 length);

	void BopPsiSender::init(u64 n, u64 statSec, Channel & chl0, SSOtExtSender& ots, block seed)
	{
		init(n, statSec, { &chl0 }, ots, seed);
	}

	void BopPsiSender::init(u64 n, u64 statSec, const std::vector<Channel*>& chls, SSOtExtSender& otSend, block seed)
	{
		mStatSecParam = statSec;
		mN = n;
		mNumStash = get_stash_size(n);

		// we need a random hash function, so both commit to a seed and then decommit later
		PRNG prngHashing(seed);
		block myHashSeeds;
		myHashSeeds = prngHashing.get_block();
		auto& chl0 = *chls[0];
		chl0.asyncSend(&myHashSeeds, sizeof(block));


		block theirHashingSeeds;
		chl0.asyncRecv(&theirHashingSeeds, sizeof(block));

		// init Simple hash
		mBins.init(n);

		mPsiRecvSSOtMessages.resize(mBins.mBinCount + mNumStash);

		//do base OT
		if (otSend.hasBaseSSOts() == false)
		{
			//Timer timer;
			BaseSSOT baseSSOTs(chl0, OTRole::Receiver);
			baseSSOTs.exec_base(prngHashing);
			baseSSOTs.check();
			otSend.setBaseSSOts(baseSSOTs.receiver_outputs, baseSSOTs.receiver_inputs);
			//	gTimer.setTimePoint("s baseDOne");
			mSSOtChoice = baseSSOTs.receiver_inputs;
			//Log::out << gTimer;
		}

		mHashingSeed = myHashSeeds ^ theirHashingSeeds;

		otSend.Extend(mBins.mBinCount + mNumStash, mPsiRecvSSOtMessages, chl0);
		//gTimer.setTimePoint("s InitS.extFinished");
	}


	void BopPsiSender::sendInput(std::vector<block>& inputs, Channel & chl)
	{
		sendInput(inputs, { &chl });
	}

	void BopPsiSender::sendInput(std::vector<block>& inputs, const std::vector<Channel*>& chls)
	{
		if (inputs.size() != mN)
			throw std::runtime_error("rt error at " LOCATION);


		//gTimer.setTimePoint("OnlineS.start");
		PRNG prng(ZeroBlock);


		auto& chl = *chls[0];


		//PRNG hSeeds(mHashingSeed);	
		//Log::out << "mHashingSeed" << mHashingSeed << Log::endl;

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
		}

		//gTimer.setTimePoint("OnlineS.hashingDone");


		SHA1 sha1;
		u8 hashBuff1[SHA1::HashSize];

		u64 numHashs = mBins.insertItems(aesHashBuffs);
		//mBins.print();

		//gTimer.setTimePoint("OnlineS.inserted");

		auto binStart = 0;
		auto binEnd = mBins.mBinCount;

		//gTimer.setTimePoint("s recvSend: Start");


		auto& blk448Choice = mSSOtChoice.getArrayView<blockBop>()[0];

		blockBop codeWord;

		ByteStream theirMasksBuff;
		u64 maskSize = get_mask_size(inputs.size());
		u64 codeWordSize = get_codeword_size(inputs.size()); //by byte


		std::unique_ptr<ByteStream> myMaskBuff(new ByteStream());
		u64 cntMask = (3 + mNumStash)*mBins.mN;
		myMaskBuff->resize(cntMask* maskSize);

		//create permute array to add my mask in permuted position later
		std::vector<u64>permute(cntMask);
		for (u64 i = 0; i < cntMask; i++)
			permute[i] = i;

		//permute position
		std::shuffle(permute.begin(), permute.end(), prng);

		auto myMaskByteIter = myMaskBuff->data();
		int cnt = 0;

		// for each of my bins, receive the OT value, compute mask, send to receiver.
		TODO("run in parallel");
		for (u64 stepIdx = binStart; stepIdx < binEnd; stepIdx += stepSize)
		{
			// compute the  size of the current step and the end index
			auto currentStepSize = std::min(stepSize, binEnd - stepIdx);
			auto stepEnd = stepIdx + currentStepSize;

			// receive their OT values.
			chl.recv(theirMasksBuff);

			// check the size
			if (theirMasksBuff.size() != sizeof(blockBop)*currentStepSize)
				throw std::runtime_error("rt error at " LOCATION);
			
			auto theirMasks = theirMasksBuff.getArrayView<blockBop>();

			// loop all the bins in this step.
			for (u64 bIdx = stepIdx, j = 0; bIdx < stepEnd; ++bIdx, ++j)
			{
				// current bin.
				auto& bin = mBins.mBins[bIdx];

				//Log::out << "bIdx " << bIdx << "  correction " << theirMasks[j] << Log::endl;
				
				// for each item, hash it, encode then hash it again. 
				for (u64 i = 0; i < bin.mSize; ++i)
				{

					codeWord.elem[0] = aesHashBuffs[0][bin.mItems[i].mIdx];
					codeWord.elem[1] = aesHashBuffs[1][bin.mItems[i].mIdx];
					codeWord.elem[2] = aesHashBuffs[2][bin.mItems[i].mIdx];
					codeWord.elem[3] = aesHashBuffs[3][bin.mItems[i].mIdx];

					auto sum = mPsiRecvSSOtMessages[bIdx] ^ ((theirMasks[j] ^ codeWord) & blk448Choice);

					sha1.Reset();
					sha1.Update((u8*)&bIdx, sizeof(bIdx));
					sha1.Update((u8*)&sum, codeWordSize);
					sha1.Final(hashBuff1);
					//gTimer.setTimePoint("sha1.done");
					//Log::out << "  hash " << hexString(hashBuff1, maskSize) << " " << hashBuff1 - myMaskBuff->data() << Log::endl;

					//if (bIdx == 0)
						//Log::out << "s  hash[" << bIdx<<"]:" << hexString(hashBuff1, maskSize) << Log::endl;
					// copy it into the buffer at permuted position
					memcpy(myMaskBuff->data() + permute[cnt] * maskSize, hashBuff1, maskSize);
					cnt++;
				}

			}
		}

		//gTimer.setTimePoint("OnlineS.BinsSent");

		//Log::out << "cnt= " << cnt << Log::endl;
		//Log::out << "myMaskBuff->size()= " <<  myMaskBuff->size()/ maskSize << Log::endl;


		chl.recv(theirMasksBuff);

		auto theirMasks = theirMasksBuff.getArrayView<blockBop>();

		if (theirMasks.size() != mNumStash)
			throw std::runtime_error("rt error at " LOCATION);

		// now compute mask for each of the stash elements
		//Log::out << "mNumStash= " << mNumStash << Log::endl;  
		for (u64 stashIdx = 0, otIdx = mBins.mBinCount; stashIdx < mNumStash; ++stashIdx, ++otIdx)
		{			
			for (u64 stepIdx = 0; stepIdx < inputs.size(); stepIdx += stepSize)
			{
				auto currentStepSize = std::min(stepSize, inputs.size() - stepIdx);
				auto stepEnd = stepIdx + currentStepSize;

				for (u64 i = stepIdx; i < stepEnd; ++i)
				{
					codeWord.elem[0] = aesHashBuffs[0][i];
					codeWord.elem[1] = aesHashBuffs[1][i];
					codeWord.elem[2] = aesHashBuffs[2][i];
					codeWord.elem[3] = aesHashBuffs[3][i];

					codeWord = mPsiRecvSSOtMessages[stashIdx] ^ ((theirMasks[stashIdx] ^ codeWord) & blk448Choice);


					sha1.Reset();
					sha1.Update((u8*)&otIdx, sizeof(otIdx));
					sha1.Update((u8*)&codeWord, codeWordSize);
					sha1.Final(hashBuff1);

					// copy it into the buffer in permuted pos
					memcpy(myMaskBuff->data() + permute[cnt] * maskSize, hashBuff1, maskSize);
					cnt++;

				}
			}
		}

		//Log::out << "cnt= " << cnt << Log::endl;
		//Log::out << "myMaskBuff->size()= " << myMaskBuff->size() / maskSize << Log::endl;
		//Log::out << "cntMask= " << cntMask << Log::endl;

		//fill the rest of the buffer with dummy. 
		myMaskByteIter = myMaskBuff->data();
		auto dummyMasksSize = maskSize * (3 * mN - numHashs);
		const block rndBlk = _mm_set_epi32(123213, 43154243, 3241248, 57657453);
		PRNG prng1(rndBlk);
		prng1.get_u8s(myMaskByteIter, dummyMasksSize);
		myMaskByteIter += dummyMasksSize;

		if (cntMask != myMaskBuff->size() / maskSize)
		{
			Log::out << "myMaskByteIter != myMaskBuff->data() + myMaskBuff->size()" << Log::endl;
			throw std::runtime_error("rt error at " LOCATION);
		}
		chl.asyncSend(std::move(myMaskBuff));
		//gTimer.setTimePoint("s recvSend: done");
	//	Log::out << gTimer << Log::endl;
	}

}


