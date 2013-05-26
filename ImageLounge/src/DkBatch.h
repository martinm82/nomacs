/*******************************************************************************************************
 DkBatch.h
 Created on:	26.05.2013
 
 nomacs is a fast and small image viewer with the capability of synchronizing multiple instances
 
 Copyright (C) 2011-2013 Markus Diem <markus@nomacs.org>
 Copyright (C) 2011-2013 Stefan Fiel <stefan@nomacs.org>
 Copyright (C) 2011-2013 Florian Kleber <florian@nomacs.org>

 This file is part of nomacs.

 nomacs is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 nomacs is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.

 *******************************************************************************************************/

#pragma once

#include <QDialog>
#include <QDialogButtonBox>
#include <QPushButton>
#include <QVBoxLayout>

#include "DkWidgets.h"

namespace nmc {

class DkFileSelection : public QLabel {
	Q_OBJECT

public:
	DkFileSelection(QWidget* parent = 0, Qt::WindowFlags f = 0);

	QDir getDir() {
		return cDir;
	};

	void setDir(const QDir& dir) {
		cDir = dir;
		indexDir();
	};

public slots:
	void browse();
	void indexDir();

signals:
	void dirSignal(const QString& dir);

protected:
	void createLayout();

	QDir cDir;
};

class DkBatchDialog : public QDialog {
	Q_OBJECT

public:
	DkBatchDialog(QWidget* parent = 0, Qt::WindowFlags f = 0);

	DkFileSelection* getFileSelection() {
		return fileSelection;
	};

public slots:
	virtual void accept();
	void setInputDir(const QString& dirName);
	void setOutputDir(const QString& dirName);

protected:
	void createLayout();

	DkFileSelection* fileSelection;

	QLabel* inputDirLabel;
	QLabel* outputDirLabel;
};

};
