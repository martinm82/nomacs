/*******************************************************************************************************
 DkBatch.cpp
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

#include "DkBatch.h"

namespace nmc {

// File Model --------------------------------------------------------------------
DkFileModel::DkFileModel(QObject* parent) : QAbstractListModel(parent) {

}

QVariant DkFileModel::data(const QModelIndex &index, int role) const {

	//if (!index.isValid())
	//	return QVariant();

	// nothing is happening here... damn
	qDebug() << "returning data";
	//if (fileData.at(index.row()))
		return 5;



	//return QVariant();
}

int DkFileModel::rowCount(const QModelIndex &parent) const {

	qDebug() << "returning: " << fileData.size();
	return fileData.size();
}

void DkFileModel::setFileList(const QStringList& list) {

	for (int idx = 0; idx < fileData.size(); idx++) {

		if (fileData.at(idx))
			delete fileData.at(idx);
	}

	fileData.clear();

	for (int idx = 0; idx < list.size(); idx++) {
		
		QCheckBox* cb = new QCheckBox(list.at(idx));
		fileData.append(cb);
	}

}

// File Selection --------------------------------------------------------------------
DkFileSelection::DkFileSelection(QWidget* parent /* = 0 */, Qt::WindowFlags f /* = 0 */) : QLabel(parent, f) {

	setObjectName("DkFileSelection");
	setStyleSheet("QLabel#DkFileSelection{border-radius: 5px; border: 1px solid #AAAAAA;}");
	createLayout();
	setMinimumHeight(300);
}

void DkFileSelection::createLayout() {

	QLineEdit* filterEdit = new QLineEdit();
	
	QPushButton* browseButton = new QPushButton(tr("Browse"));
	connect(browseButton, SIGNAL(clicked()), this, SLOT(browse()));

	fileModel = new DkFileModel();

	fileWidget = new QListView();
	//fileWidget->setStyleSheet("QListView::item:alternate{background: #BBB;}");
	fileWidget->setModel(fileModel);

	QGridLayout* fsLayout = new QGridLayout(this);
	fsLayout->addWidget(filterEdit, 0, 0, 1, 4);
	fsLayout->addWidget(browseButton, 0, 4);
	fsLayout->addWidget(fileWidget, 1, 0, 1, 5);	// change to 4 if we support thumbs
	//fsLayout->setRowStretch(2, 300);

	setLayout(fsLayout);
}

void DkFileSelection::browse() {

	// load system default open dialog
	QString dirName = QFileDialog::getExistingDirectory(this, tr("Open an Image Directory"),
		cDir.absolutePath());

	if (dirName.isEmpty())
		return;

	cDir = QDir(dirName);
	
	indexDir();
}

void DkFileSelection::indexDir() {

	emit dirSignal(cDir.absolutePath());
	
	//fileWidget->clear();
	
	// TODO: index directory
	// true file list
	cDir.setSorting(QDir::LocaleAware);
	QStringList fileList = cDir.entryList(DkImageLoader::fileFilters);
	fileModel->setFileList(fileList);

	//for (int idx = 0; idx < fileList.size(); idx++) {

	//	QListWidgetItem* item = new QListWidgetItem(fileList.at(idx), fileWidget);
	//	item->setCheckState(Qt::Unchecked);

	//}

}

// Batch Dialog --------------------------------------------------------------------
DkBatchDialog::DkBatchDialog(QWidget* parent, Qt::WindowFlags f) : QDialog(parent, f) {

	setWindowTitle(tr("Batch Conversion"));
	createLayout();
}

void DkBatchDialog::createLayout() {

	// File Selection
	fileSelection = new DkFileSelection(this);
	connect(fileSelection, SIGNAL(dirSignal(const QString&)), this, SLOT(setInputDir(const QString&)));
	//fileSelection->hide();
	
	DkButton* showButton = new DkButton(QIcon(":/nomacs/img/lock.png"), QIcon(":/nomacs/img/lock-unlocked.png"), "lock");
	showButton->setFixedSize(QSize(16,16));
	showButton->setObjectName("showSelectionButton");
	showButton->setCheckable(true);
	showButton->setChecked(true);
	connect(showButton, SIGNAL(toggled(bool)), fileSelection, SLOT(setVisible(bool)));

	QLabel* fileSelectionLabel = new QLabel(tr("File Selection"));
	fileSelectionLabel->setStyleSheet("QLabel{font-size: 16px;}");
	fileSelectionLabel->setAlignment(Qt::AlignBottom);
	inputDirLabel = new QLabel(tr("No Directory selected"));
	inputDirLabel->setStyleSheet("QLabel{color: #666;}");
	inputDirLabel->setAlignment(Qt::AlignBottom);

	QWidget* fileSelectionWidget = new QWidget();
	QHBoxLayout* fileSelectionLayout = new QHBoxLayout(fileSelectionWidget);
	fileSelectionLayout->setContentsMargins(0,0,0,0);
	fileSelectionLayout->addWidget(showButton);
	fileSelectionLayout->addWidget(fileSelectionLabel);
	fileSelectionLayout->addWidget(inputDirLabel);
	fileSelectionLayout->addStretch();

	// File Output
	DkButton* outputButton = new DkButton(QIcon(":/nomacs/img/lock.png"), QIcon(":/nomacs/img/lock-unlocked.png"), "lock");
	outputButton->setFixedSize(QSize(16,16));
	outputButton->setObjectName("showOutputButton");
	outputButton->setCheckable(true);
	outputButton->setChecked(false);
	//connect(outputButton, SIGNAL(toggled(bool)), fileSelection, SLOT(setVisible(bool)));	// TODO

	QLabel* outputLabel = new QLabel(tr("File Output"));
	outputLabel->setStyleSheet("QLabel{font-size: 15px;}");
	outputDirLabel = new QLabel(tr("No Directory selected"));
	outputDirLabel->setStyleSheet("QLabel{color: #333;}");

	QWidget* outputWidget = new QWidget();
	QHBoxLayout* outputLayout = new QHBoxLayout(outputWidget);
	outputLayout->setContentsMargins(0,0,0,0);
	outputLayout->addWidget(outputButton);
	outputLayout->addWidget(outputLabel);
	outputLayout->addStretch();
	
	// buttons
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	buttons->button(QDialogButtonBox::Ok)->setDefault(true);	// ok is auto-default
	buttons->button(QDialogButtonBox::Ok)->setText(tr("&OK"));
	buttons->button(QDialogButtonBox::Cancel)->setText(tr("&Cancel"));
	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout* dialogLayout = new QVBoxLayout();
	dialogLayout->addWidget(fileSelectionWidget);
	dialogLayout->addWidget(fileSelection);
	dialogLayout->addWidget(outputWidget);
	dialogLayout->addStretch();
	dialogLayout->addWidget(buttons);

	setLayout(dialogLayout);
}

void DkBatchDialog::setInputDir(const QString& dirName) {
	inputDirLabel->setText(dirName);
}

void DkBatchDialog::setOutputDir(const QString& dirName) {
	outputDirLabel->setText(dirName);
}

void DkBatchDialog::accept() {

	// TODO: batch processing here

	QDialog::accept();
}

}