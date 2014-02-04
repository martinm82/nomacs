#include "ScopedMaidEnumData.h"

using Maid::ScopedMaidEnumData;

ScopedMaidEnumData::ScopedMaidEnumData(NkMAIDEnum* e) 
		: maidEnum(e) {
}

ScopedMaidEnumData::~ScopedMaidEnumData() {
	if (maidEnum->pData != nullptr) {
		delete[] maidEnum->pData;
	}
}