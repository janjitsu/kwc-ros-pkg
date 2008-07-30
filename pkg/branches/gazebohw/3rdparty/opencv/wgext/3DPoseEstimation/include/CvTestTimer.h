#include <cxtypes.h>
#include <string>
using namespace std;
#ifndef CVTESTTIMER_H_
#define CVTESTTIMER_H_

#define DECLARE(timerName) RecordType m##timerName;
#define RESET(timerName)   do {m##timerName.reset();} while(0)

class CvTestTimer
{
public:
	CvTestTimer();
	virtual ~CvTestTimer();
	
	class RecordType {
	public:
		int64 mTime;
		int64 mCount;
		int64 mTimeStart;
		void reset() {
			mTime = 0;
			mCount = 0;
		}
	};
	
	void reset(){
		mTotal   = 0;
		mCountTotal   = 0;
		mErrNorm = 0;
		mCountErrNorm = 0;
		mJtJJtErr = 0;
		mCountJtJJtErr = 0;
		mLevMarq  = 0;
		mCountLevMarq  = 0;
		mCvLevMarq = 0;
		mCountCvLevMarq = 0;
		mLevMarqDoit = 0;
		mCountLevMarqDoit = 0;
		mIsInLier = 0;
		mCountIsInLier = 0;
		mCheckInliers = 0;
		mCountCheckInliers = 0;
		mCopyInliers = 0;
		mCountCopyInliers = 0;
		mConstructMatrices = 0;
		mCountConstructMatrices = 0;
		RESET(SVD);
		RESET(Residue);
		RESET(FwdResidue);
		RESET(LevMarq2);
		RESET(LevMarq3);
		RESET(LevMarq4);
		RESET(LevMarq5);
	}
	int64 mNumIters;
	int64 mFrequency;
	
	int64 mTotal;
	int64 mCountTotal;
	
	int64 mErrNorm;
	int64 mCountErrNorm;
	int64 mJtJJtErr;
	int64 mCountJtJJtErr;
	int64 mLevMarq;
	int64 mCountLevMarq;
	int64 mCopyInliers;
	int64 mCountCopyInliers;
	int64 mCvLevMarq;
	int64 mCountCvLevMarq;
	int64 mLevMarqDoit;
	int64 mCountLevMarqDoit;
	int64 mIsInLier;
	int64 mCountIsInLier;
	int64 mCheckInliers;
	int64 mCountCheckInliers;
	int64 mConstructMatrices;
	int64 mCountConstructMatrices;
	
	DECLARE(SVD);
	DECLARE(Residue);
	DECLARE(FwdResidue);
	DECLARE(LevMarq2);
	DECLARE(LevMarq3);
	DECLARE(LevMarq4);
	DECLARE(LevMarq5);
	
	void printStat();
	void printStat(const char* title, int64 val, int64 count);
	static inline CvTestTimer& getTimer() {
		return _singleton;
	}
	static CvTestTimer _singleton;
};

#define CvTestTimerStart(timerName) \
	{ int64 _CvTestTimer_##timerName = cvGetTickCount(); \
	  CvTestTimer::getTimer().mCount##timerName++;

#define CvTestTimerEnd(timerName) \
	CvTestTimer::getTimer().m##timerName += cvGetTickCount() - _CvTestTimer_##timerName;}


#define CvTestTimerStart2(timerName) \
	do { CvTestTimer::getTimer().m##timerName.mTimeStart = cvGetTickCount(); \
	  CvTestTimer::getTimer().m##timerName.mCount++;} while(0)

#define CvTestTimerEnd2(timerName) \
	do { CvTestTimer::getTimer().m##timerName.mTime += \
			cvGetTickCount() - CvTestTimer::getTimer().m##timerName.mTimeStart;} while (0)


#endif /*CVTESTTIMER_H_*/
