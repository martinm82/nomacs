#pragma once

#include <memory>
#include <array>
#include <set>
#include <vector>
#include <functional>
#include <QMutex>
#include <QStringList>
#include "MaidObject.h"

std::string makePictureFilename(NkMAIDDataInfo* dataInfo, NkMAIDFileInfo* fileInfo = nullptr);
void CALLPASCAL CALLBACK eventProc(NKREF ref, ULONG ulEvent, NKPARAM data);
NKERROR CALLPASCAL CALLBACK dataProc(NKREF ref, LPVOID info, LPVOID data);
void CALLPASCAL CALLBACK completionProc(LPNkMAIDObject pObject, ULONG ulCommand, ULONG ulParam, ULONG ulDataType, NKPARAM data, NKREF refComplete, NKERROR nResult);

namespace nmc {

class MaidFacade {
public:
	struct StringValues {
		std::vector<std::string> values;
		size_t currentValue;
	};

	struct UnsignedValues {
		std::vector<uint32_t> values;
		size_t currentValue;
	};

	struct DataProcData {
		DataProcData() : buffer(nullptr), offset(0), totalLines(0), id(-1) {}
		char* buffer;
		size_t offset;
		size_t totalLines;
		long id;
	};

	struct CompletionProcData {
		CompletionProcData() : result(kNkMAIDResult_NotInitialized), count(nullptr), data(nullptr) {}
		NKERROR result;
		ULONG* count;
		DataProcData* data;
	};

	typedef std::pair<MaidFacade::StringValues, bool> MaybeStringValues;
	typedef std::pair<MaidFacade::UnsignedValues, bool> MaybeUnsignedValues;

	MaidFacade();
	~MaidFacade() {}

	// callbacks are public because they have to be called from a function outside the class
	std::function<void(unsigned long)> capValueChangeCallback;

	void init();
	void setCapValueChangeCallback(std::function<void(uint32_t)> capValueChangeCallback);
	std::set<uint32_t> listDevices();
	void openSource(ULONG id);
	bool checkCameraType();
	void closeModule();
	void closeSource();
	void closeEverything();
	bool isSourceAlive();
	MaybeStringValues readAperture();
	MaybeStringValues readSensitivity();
	MaybeStringValues readShutterSpeed();
	MaybeUnsignedValues readExposureMode();
	MaybeStringValues getAperture();
	MaybeStringValues getSensitivity();
	MaybeStringValues getShutterSpeed();
	MaybeUnsignedValues getExposureMode();
	bool setAperture(size_t newValue);
	bool setSensitivity(size_t newValue);
	bool setShutterSpeed(size_t newValue);
	bool setExposureMode(size_t newValue);
	bool isLensAttached();
	bool shoot(bool withAf = false);
	bool acquireItemObjects(const std::unique_ptr<Maid::MaidObject>& sourceObject);
	std::pair<QStringList, size_t> toQStringList(const StringValues&);

private:
	std::unique_ptr<Maid::MaidObject> moduleObject;
	std::unique_ptr<Maid::MaidObject> sourceObject;
	MaybeStringValues aperture;
	MaybeStringValues sensitivity;
	MaybeStringValues shutterSpeed;
	MaybeUnsignedValues exposureMode;
	QMutex mutex;
	bool lensAttached;
	
	//void closeChildren(std::unique_ptr<Maid::MaidObject> mo);
	MaybeStringValues readPackedStringCap(ULONG capId);
	MaybeUnsignedValues readUnsignedEnumCap(ULONG capId);
	bool writeEnumCap(ULONG capId, size_t newValue);
	void sourceIdleLoop(ULONG* count);
	bool setMaybeStringEnumValue(std::pair<StringValues, bool>& theMaybeValue, ULONG capId, size_t newValue);
	bool setMaybeUnsignedEnumValue(std::pair<UnsignedValues, bool>& theMaybeValue, ULONG capId, size_t newValue);
};

};