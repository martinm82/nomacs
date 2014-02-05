#include <iostream>
#include <fstream>
#include <sstream>
#include <chrono>
#include <thread>
#include <QDebug>
#include "MaidFacade.h"
#include "MaidUtil.h"
#include "MaidError.h"

using nmc::MaidFacade;
using Maid::MaidUtil;
using Maid::MaidObject;

MaidFacade::MaidFacade(std::function<void(ULONG)> capValueChangeCallback) 
	: capValueChangeCallback(capValueChangeCallback), lensAttached(false) {
}

/*!
 * throws InitError, MaidError
 */
void MaidFacade::init() {
	MaidUtil::getInstance().loadLibrary();
	qDebug() << "Loaded MAID library";
	MaidUtil::getInstance().initMAID();
	qDebug() << "Initialized MAID";

	// create module object
	moduleObject.reset(MaidObject::create(0, nullptr));
	moduleObject->enumCaps();
	qDebug() << "MAID Module Object created";
	// set callbacks
	moduleObject->setEventCallback(this, eventProc);
	//moduleObject->setProgressCallback(progressProc);
	//moduleObject->setUIRequestCallback(uiRequestProc);
	// set ModuleMode
	//if (moduleObject->hasCapOperation(kNkMAIDCapability_ModuleMode, kNkMAIDCapOperation_Set)) {
	//	moduleObject->capSet(kNkMAIDCapability_ModuleMode, kNkMAIDDataType_Unsigned, (NKPARAM) kNkMAIDModuleMode_Controller);
	//} else {
	//	qDebug() << "kNkMAIDCapability_ModuleMode can not be set";
	//}
}


void CALLPASCAL CALLBACK eventProc(NKREF ref, ULONG eventType, NKPARAM data) {
	MaidFacade* maidFacade = (MaidFacade*) ref;

	switch (eventType) {
	case kNkMAIDEvent_AddChild:
		qDebug() << "A MAID child was added";
		break;
	case kNkMAIDEvent_RemoveChild:
		qDebug() << "A MAID child was removed";
		break;
	case kNkMAIDEvent_WarmingUp:
		// The Type0007 Module does not use this event.
		break;
	case kNkMAIDEvent_WarmedUp:
		// The Type0007 Module does not use this event.
		break;
	case kNkMAIDEvent_CapChange:
		// TODO re-enumerate the capabilities
		maidFacade->capValueChangeCallback(data);
		break;
	case kNkMAIDEvent_CapChangeValueOnly:
		// LOG(INFO) << "The value of a Capability (CapID=0x " << std::hex << data << ") was changed.";
		maidFacade->capValueChangeCallback(data);
		break;
	case kNkMAIDEvent_OrphanedChildren:
		// TODO close children (source objects)
		break;
	default:
		qDebug() << "Unknown Event in a MaidObject.";
	}
}

/*!
 * If fileInfo is nullptr, it is assumed that it is a raw picture
 */
std::string makePictureFilename(NkMAIDDataInfo* dataInfo, NkMAIDFileInfo* fileInfo) {
	std::string prefix;
	std::string ext;

	if (dataInfo->ulType & kNkMAIDDataObjType_Image) {
		prefix = "Image";
	} else if (dataInfo->ulType & kNkMAIDDataObjType_Thumbnail) {
		prefix = "Thumb";
	} else {
		prefix = "Unknown";
	}

	if (fileInfo == nullptr) {
		ext = "raw";
	} else {
		switch (fileInfo->ulFileDataType) {
		case kNkMAIDFileDataType_JPEG:
			ext = "jpg";
			break;
		case kNkMAIDFileDataType_TIFF:
			ext = "tif";
			break;
		case kNkMAIDFileDataType_NIF:
			ext = "nef";
			break;
		case kNkMAIDFileDataType_NDF:
			ext = "ndf";
			break;
		default:
			ext = "dat";
		}
	}

	std::stringstream filenameStream;
	std::ifstream testFileIn;
	unsigned int i = 0;
	// test file names
	while (true) {
		filenameStream.str("");
		filenameStream << prefix << i << "." << ext;
		testFileIn.open(filenameStream.str());
		if (!testFileIn.good()) {
			testFileIn.close();
			break;
		}

		testFileIn.close();
		++i;
	}

	return filenameStream.str();
}

NKERROR CALLPASCAL CALLBACK dataProc(NKREF ref, LPVOID info, LPVOID data) {
	NkMAIDDataInfo* dataInfo = static_cast<NkMAIDDataInfo*>(info);
	NkMAIDFileInfo* fileInfo = static_cast<NkMAIDFileInfo*>(info);
	NkMAIDImageInfo* imageInfo = static_cast<NkMAIDImageInfo*>(info);
	auto r = static_cast<MaidFacade::DataProcData*>(ref);
	
	if (dataInfo->ulType & kNkMAIDDataObjType_File) {
		// initialize buffer
		if (r->offset == 0 && r->buffer == nullptr) {
			r->buffer = new char[fileInfo->ulTotalLength];
		}

		memcpy(r->buffer + r->offset, data, fileInfo->ulLength);
		r->offset += fileInfo->ulLength;

		if (r->offset >= fileInfo->ulTotalLength) {
			// file delivery is finished, write to disk

			std::string filename = makePictureFilename(dataInfo, fileInfo);
			std::ofstream outFile;

			outFile.open(filename, std::ios::out | std::ios::binary);
			if (!outFile.good() || !outFile.is_open()) {
				return kNkMAIDResult_UnexpectedError;
			}

			outFile.write(r->buffer, fileInfo->ulTotalLength);
			delete[] r->buffer;
			r->buffer = nullptr;
			r->offset = 0;
			outFile.close();
		}
	} else { // image
		unsigned long totalSize = imageInfo->ulRowBytes * imageInfo->szTotalPixels.h;
		if (r->offset == 0 && r->buffer == nullptr) {
			r->buffer = new char[totalSize];
		}
		unsigned long byte = imageInfo->ulRowBytes * imageInfo->rData.h;
		memcpy(r->buffer + r->offset, data, byte);
		r->offset += byte;

		if (r->offset >= totalSize) {
			std::string filename = makePictureFilename(dataInfo, nullptr);
			std::ofstream outFile;

			outFile.open(filename, std::ios::out | std::ios::binary);
			if (!outFile.good() || !outFile.is_open()) {
				return kNkMAIDResult_UnexpectedError;
			}

			outFile.write(r->buffer, totalSize);
			delete[] r->buffer;
			r->buffer = nullptr;
			r->offset = 0;
			outFile.close();
		}
	}

	return kNkMAIDResult_NoError;
}

void CALLPASCAL CALLBACK completionProc(
		LPNkMAIDObject object,
		ULONG command,
		ULONG param,
		ULONG dataType,
		NKPARAM data,
		NKREF refComplete,
		NKERROR result) {

	auto complData = static_cast<MaidFacade::CompletionProcData*>(refComplete);
	complData->result = result;
	(*complData->count)++;

	// if the operation is aquire, free the memory
	if(command == kNkMAIDCommand_CapStart && param == kNkMAIDCapability_Acquire) {
		auto data = static_cast<MaidFacade::DataProcData*>(complData->data);
		if (data != nullptr) {
			if (data->buffer != nullptr) {
				delete[] data->buffer;
			}
			delete data;
		}
	}
	
	if (complData != nullptr) {
		delete complData;
	}
}