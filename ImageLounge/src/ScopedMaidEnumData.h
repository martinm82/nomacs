#ifndef _SCOPEDMAIDENUMDATA_H
#define _SCOPEDMAIDENUMDATA_H

#include "Maid3.h"

namespace Maid {

/*!
 * Holds a maid enum and cleans its data (RAII). 
 * Should never live longer than the original enum.
 */
class ScopedMaidEnumData {
public:
	ScopedMaidEnumData(NkMAIDEnum* e);
	~ScopedMaidEnumData();
	
	NkMAIDEnum* getEnum() {
		return maidEnum;
	}
private:
	NkMAIDEnum* maidEnum;
};

}

#endif