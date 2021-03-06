/*******************************************************************************************************
 DkNoMacs.cpp
 Created on:	26.10.2014
 
 nomacs is a fast and small image viewer with the capability of synchronizing multiple instances
 
 Copyright (C) 2011-2014 Markus Diem <markus@nomacs.org>
 Copyright (C) 2011-2014 Stefan Fiel <stefan@nomacs.org>
 Copyright (C) 2011-2014 Florian Kleber <florian@nomacs.org>

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
#include "DkProcess.h"
#include "DkDialog.h"
#include "DkWidgets.h"
#include "DkThumbsWidgets.h"
#include "DkUtils.h"
#include "DkImage.h"
#include "DkSettings.h"
#include "DkMessageBox.h"

#pragma warning(push, 0)	// no warnings from includes - begin
#include <QDialogButtonBox>
#include <QLabel>
#include <QListView>
#include <QLineEdit>
#include <QFileDialog>
#include <QGroupBox>
#include <QComboBox>
#include <QButtonGroup>
#include <QProgressBar>
#include <QTextEdit>
#include <QCheckBox>
#include <QDoubleSpinBox>
#include <QHBoxLayout>
#include <QRadioButton>
#include <QMessageBox>
#include <QApplication>
#include <QTextBlock>
#include <QDropEvent>
#include <QMimeData>
#pragma warning(pop)		// no warnings from includes - end

namespace nmc {

// DkBatchWidget --------------------------------------------------------------------
DkBatchWidget::DkBatchWidget(QString titleString, QString headerString, QWidget* parent, Qt::WindowFlags f) : QWidget(parent, f) {
	this->titleString = titleString;
	this->headerString = headerString;
	createLayout();
}

void DkBatchWidget::createLayout() {
	
	showButton = new DkButton(QIcon(":/nomacs/img/minus.png"), QIcon(":/nomacs/img/plus.png"), tr("Plus"));
	showButton->setFixedSize(QSize(16,16));
	showButton->setObjectName("showSelectionButton");
	showButton->setCheckable(true);
	showButton->setChecked(true);

	titleLabel = new QLabel(titleString);
	titleLabel->setObjectName("DkBatchTitle");
	titleLabel->setAlignment(Qt::AlignBottom);
	headerLabel = new QLabel(headerString);
	headerLabel->setObjectName("DkDecentInfo");
	headerLabel->setAlignment(Qt::AlignBottom);
	
	QWidget* headerWidget = new QWidget(this);
	QHBoxLayout* headerWidgetLayout = new QHBoxLayout(headerWidget);
	headerWidgetLayout->setContentsMargins(0,0,0,0);
	headerWidgetLayout->addWidget(showButton);
	headerWidgetLayout->addWidget(titleLabel);
	headerWidgetLayout->addWidget(headerLabel);
	headerWidgetLayout->addStretch();

	batchWidgetLayout = new QVBoxLayout(this);
	batchWidgetLayout->setContentsMargins(0,0,0,0);
	batchWidgetLayout->addWidget(headerWidget);
	//batchWidgetLayout->addWidget(contentWidget);
	//batchWidgetLayout->addStretch();
	setLayout(batchWidgetLayout);
}

void DkBatchWidget::setContentWidget(QWidget* batchContent) {
	
	this->batchContent = dynamic_cast<DkBatchContent*>(batchContent);

	batchWidgetLayout->addWidget(batchContent);
	connect(showButton, SIGNAL(toggled(bool)), batchContent, SLOT(setVisible(bool)));
	connect(batchContent, SIGNAL(newHeaderText(QString)), this, SLOT(setHeader(QString)));
}

QWidget* DkBatchWidget::contentWidget() const {
	
	return dynamic_cast<QWidget*>(batchContent);
}

void DkBatchWidget::showContent(bool) {

	showButton->click();
	//contentWidget()->setVisible(show);
}

void DkBatchWidget::setTitle(QString titleString) {
	this->titleString = titleString;
	titleLabel->setText(titleString);
}

void DkBatchWidget::setHeader(QString headerString) {
	this->headerString = headerString;
	headerLabel->setText(headerString);
}

// DkInputTextEdit --------------------------------------------------------------------
DkInputTextEdit::DkInputTextEdit(QWidget* parent /* = 0 */) : QTextEdit(parent) {

	setAcceptDrops(true);
	connect(this, SIGNAL(textChanged()), this, SIGNAL(fileListChangedSignal()));
}

void DkInputTextEdit::appendFiles(const QStringList& fileList) {

	QStringList cFileList = getFileList();
	QStringList newFiles;

	// unique!
	for (QString cStr : fileList) {

		if (!cFileList.contains(cStr))
			newFiles.append(cStr);
	}

	if (!newFiles.empty()) {
		append(newFiles.join("\n"));
		fileListChangedSignal();
	}
}

void DkInputTextEdit::appendDir(const QDir& newDir, bool recursive) {

	if (recursive) {
		qDebug() << "adding recursive...";
		QDir tmpDir = newDir;
		QFileInfoList subDirs = tmpDir.entryInfoList(QDir::NoDotAndDotDot | QDir::Dirs);

		for (QFileInfo cDir : subDirs)
			appendDir(QDir(cDir.absoluteFilePath()), recursive);
	}

	QDir tmpDir = newDir;
	tmpDir.setSorting(QDir::LocaleAware);
	QFileInfoList fileList = tmpDir.entryInfoList(DkSettings::app.fileFilters);
	QStringList strFileList;

	for (QFileInfo entry : fileList) {
		strFileList.append(entry.absoluteFilePath());
	}

	qDebug() << "appending " << strFileList.size() << " files";

	appendFiles(strFileList);
}

void DkInputTextEdit::appendFromMime(const QMimeData* mimeData) {

	if (!mimeData || !mimeData->hasUrls())
		return;

	QStringList cFiles;

	for (QUrl url : mimeData->urls()) {

		QFileInfo cFile = DkUtils::urlToLocalFile(url);

		if (cFile.isDir()) {
			QDir newDir(cFile.absoluteFilePath());
			appendDir(newDir, false);	// TODO: ask user for recursive??
		}
		else if (cFile.exists() && DkUtils::isValid(cFile)) {
			cFiles.append(cFile.absoluteFilePath());
		}
	}

	if (!cFiles.empty())
		appendFiles(cFiles);
}

void DkInputTextEdit::insertFromMimeData(const QMimeData* mimeData) {

	appendFromMime(mimeData);
	QTextEdit::insertFromMimeData(mimeData);
}

void DkInputTextEdit::dragEnterEvent(QDragEnterEvent *event) {

	QTextEdit::dragEnterEvent(event);

	if (event->source() == this)
		event->acceptProposedAction();
	else if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}

void DkInputTextEdit::dragMoveEvent(QDragMoveEvent *event) {

	QTextEdit::dragMoveEvent(event);

	if (event->source() == this)
		event->acceptProposedAction();
	else if (event->mimeData()->hasUrls())
		event->acceptProposedAction();
}


void DkInputTextEdit::dropEvent(QDropEvent *event) {
	
	if (event->source() == this) {
		event->accept();
		return;
	}

	appendFromMime(event->mimeData());

	QTextEdit::dropEvent(event);
}

QStringList DkInputTextEdit::getFileList() const {

	QStringList fileList;
	QString textString;
	QTextStream textStream(&textString);
	textStream << toPlainText();

	QString line;
	do
	{
		line = textStream.readLine();	// we don't want to get into troubles with carriage returns of different OS
		if (!line.isNull() && !line.trimmed().isEmpty())
			fileList.append(line);
	} while(!line.isNull());

	return fileList;
}

void DkInputTextEdit::clear() {
	
	resultList.clear();
	QTextEdit::clear();
}

// File Selection --------------------------------------------------------------------
DkFileSelection::DkFileSelection(QWidget* parent /* = 0 */, Qt::WindowFlags f /* = 0 */) : QWidget(parent, f) {

	this->hUserInput = false;
	this->rUserInput = false;
	loader = QSharedPointer<DkImageLoader>(new DkImageLoader());
	
	setObjectName("DkFileSelection");
	createLayout();
	setMinimumHeight(300);

}

void DkFileSelection::createLayout() {
	
	directoryEdit = new DkDirectoryEdit(this);

	//QPushButton* browseButton = new QPushButton(tr("Browse"));
	//connect(browseButton, SIGNAL(clicked()), this, SLOT(browse()));

	QWidget* upperWidget = new QWidget(this);
	QGridLayout* upperWidgetLayout = new QGridLayout(upperWidget);
	upperWidgetLayout->setContentsMargins(0,0,0,0);
	//upperWidgetLayout->addWidget(browseButton, 0, 0);
	upperWidgetLayout->addWidget(directoryEdit, 0, 1);

	inputTextEdit = new DkInputTextEdit(this);

	resultTextEdit = new QTextEdit(this);
	resultTextEdit->setReadOnly(true);
	resultTextEdit->setVisible(false);

	thumbScrollWidget = new DkThumbScrollWidget(this);
	thumbScrollWidget->setVisible(true);
	thumbScrollWidget->getThumbWidget()->setImageLoader(loader);

	infoLabel = new QLabel(tr("No Files Selected"), this);

	// add explorer
	explorer = new DkExplorer(tr("File Explorer"));
	//explorer->setTitleBarWidget(new QWidget(explorer));	// no title bar
	explorer->getModel()->setFilter(QDir::Dirs|QDir::Drives|QDir::NoDotAndDotDot|QDir::AllDirs);
	explorer->getModel()->setNameFilters(QStringList());
	explorer->setMaximumWidth(300);

	QStringList folders = DkSettings::global.recentFiles;

	if (folders.size() > 0)
		explorer->setCurrentPath(folders[0]);

	// tab widget
	inputTabs = new QTabWidget(this);
	inputTabs->addTab(thumbScrollWidget,  QIcon(":/nomacs/img/thumbs-view.png"), tr("Thumbnails"));
	inputTabs->addTab(inputTextEdit, QIcon(":/nomacs/img/batch-processing.png"), tr("File List"));

	QGridLayout* widgetLayout = new QGridLayout(this);
	widgetLayout->addWidget(explorer, 0, 0, 3, 1);
	widgetLayout->addWidget(upperWidget, 0, 1);
	widgetLayout->addWidget(inputTabs, 1, 1);
	widgetLayout->addWidget(infoLabel, 2, 1);
	setLayout(widgetLayout);

	connect(thumbScrollWidget->getThumbWidget(), SIGNAL(selectionChanged()), this, SLOT(selectionChanged()));
	connect(thumbScrollWidget, SIGNAL(batchProcessFilesSignal(const QStringList&)), inputTextEdit, SLOT(appendFiles(const QStringList&)));
	connect(thumbScrollWidget, SIGNAL(updateDirSignal(QDir)), this, SLOT(setDir(QDir)));
	connect(thumbScrollWidget, SIGNAL(filterChangedSignal(const QString &)), loader.data(), SLOT(setFolderFilter(const QString&)), Qt::UniqueConnection);
	
	connect(inputTextEdit, SIGNAL(fileListChangedSignal()), this, SLOT(selectionChanged()));

	connect(directoryEdit, SIGNAL(textChanged(QString)), this, SLOT(emitChangedSignal()));
	connect(directoryEdit, SIGNAL(directoryChanged(QDir)), this, SLOT(setDir(QDir)));
	connect(explorer, SIGNAL(openDir(QDir)), this, SLOT(setDir(QDir)));
	connect(loader.data(), SIGNAL(updateDirSignal(QVector<QSharedPointer<DkImageContainerT> >)), thumbScrollWidget, SLOT(updateThumbs(QVector<QSharedPointer<DkImageContainerT> >)));

}

void DkFileSelection::changeTab(int tabIdx) const {

	if (tabIdx < 0 || tabIdx >= inputTabs->count())
		return;

	inputTabs->setCurrentIndex(tabIdx);
}

void DkFileSelection::updateDir(QVector<QSharedPointer<DkImageContainerT> > thumbs) {
	qDebug() << "emitting updateDirSignal";
	emit updateDirSignal(thumbs);
}

void DkFileSelection::setVisible(bool visible) {

	QWidget::setVisible(visible);
	thumbScrollWidget->getThumbWidget()->updateLayout();
}

void DkFileSelection::browse() {

	// load system default open dialog
	QString dirName = QFileDialog::getExistingDirectory(this, tr("Open an Image Directory"),
		cDir.absolutePath());

	if (dirName.isEmpty())
		return;

	setDir(QDir(dirName));
}

QString DkFileSelection::getDir() const {

	return directoryEdit->existsDirectory() ? QDir(directoryEdit->text()).absolutePath() : "";
}

QStringList DkFileSelection::getSelectedFiles() const {
	
	QStringList textList = inputTextEdit->getFileList();

	if (textList.empty())
		return thumbScrollWidget->getThumbWidget()->getSelectedFiles();
	else
		return textList;
}

QStringList DkFileSelection::getSelectedFilesBatch() {

	QStringList textList = inputTextEdit->getFileList();

	if (textList.empty()) {
		textList = thumbScrollWidget->getThumbWidget()->getSelectedFiles();
		inputTextEdit->appendFiles(textList);
	}

	return textList;
}


DkInputTextEdit* DkFileSelection::getInputEdit() const {

	return inputTextEdit;
}

void DkFileSelection::setFileInfo(QFileInfo file) {

	setDir(QDir(file.absoluteFilePath()));
}

void DkFileSelection::setDir(QDir dir) {

	explorer->setCurrentPath(QFileInfo(dir, ""));

	cDir = dir;
	qDebug() << "setting directory to:" << dir;
	directoryEdit->setText(cDir.absolutePath());
	emit newHeaderText(cDir.absolutePath());
	emit updateInputDir(cDir);
	loader->setDir(cDir);
	thumbScrollWidget->updateThumbs(loader->getImages());
}

void DkFileSelection::selectionChanged() {

	if (getSelectedFiles().empty())
		infoLabel->setText(tr("No Files Selected"));
	else if (getSelectedFiles().size() == 1)
		infoLabel->setText(tr("%1 File Selected").arg(getSelectedFiles().size()));
	else
		infoLabel->setText(tr("%1 Files Selected").arg(getSelectedFiles().size()));

	emit changed();
}

void DkFileSelection::emitChangedSignal() {
	
	QDir newDir = directoryEdit->text();
		
	qDebug() << "edit text newDir: " << newDir.absolutePath() << " cDir " << cDir.absolutePath();

	if (newDir.exists() && newDir.absolutePath() != cDir.absolutePath()) {
		setDir(newDir);
		emit changed();
	}
}

void DkFileSelection::setResults(const QStringList& results) {

	if (inputTabs->count() < 3) {
		inputTabs->addTab(resultTextEdit, tr("Results"));
	}

	resultTextEdit->clear();
	resultTextEdit->setHtml(results.join("<br> "));
	QTextCursor c = resultTextEdit->textCursor();
	c.movePosition(QTextCursor::End);
	resultTextEdit->setTextCursor(c);
	resultTextEdit->setVisible(true);
}

void DkFileSelection::startProcessing() {

	if (inputTabs->count() < 3) {
		inputTabs->addTab(resultTextEdit, tr("Results"));
	}

	changeTab(tab_results);
	inputTextEdit->setEnabled(false);
	resultTextEdit->clear();
}

void DkFileSelection::stopProcessing() {

	inputTextEdit->clear();
	inputTextEdit->setEnabled(true);
}

// DkFileNameWdiget --------------------------------------------------------------------
DkFilenameWidget::DkFilenameWidget(QWidget* parent) : QWidget(parent) {
	createLayout();
	showOnlyFilename();
	hasChanged = false;
}

void DkFilenameWidget::createLayout() {
	
	curLayout = new QGridLayout(this);
	curLayout->setContentsMargins(0,0,0,5);
	setMaximumWidth(500);

	cBType = new QComboBox(this);
	cBType->setSizeAdjustPolicy(QComboBox::AdjustToContents);
	cBType->insertItem(fileNameTypes_fileName, tr("Current Filename"));
	cBType->insertItem(fileNameTypes_Text, tr("Text"));
	cBType->insertItem(fileNameTypes_Number, tr("Number"));
	connect(cBType, SIGNAL(currentIndexChanged(int)), this, SLOT(typeCBChanged(int)));
	connect(cBType, SIGNAL(currentIndexChanged(int)), this, SLOT(checkForUserInput()));
	connect(cBType, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));

	cBCase = new QComboBox(this);
	cBCase->addItem(tr("Keep Case"));
	cBCase->addItem(tr("To lowercase"));
	cBCase->addItem(tr("To UPPERCASE"));
	connect(cBCase, SIGNAL(currentIndexChanged(int)), this, SLOT(checkForUserInput()));
	connect(cBCase, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));

	sBNumber = new QSpinBox(this);
	sBNumber->setValue(1);
	sBNumber->setMinimum(0);
	sBNumber->setMaximum(999);	// changes - if cbDigits->setCurrentIndex() is changed!
	connect(sBNumber, SIGNAL(valueChanged(int)), this, SIGNAL(changed()));

	cBDigits = new QComboBox(this);
	cBDigits->addItem(tr("1 digit"));
	cBDigits->addItem(tr("2 digits"));
	cBDigits->addItem(tr("3 digits"));
	cBDigits->addItem(tr("4 digits"));
	cBDigits->addItem(tr("5 digits"));
	cBDigits->setCurrentIndex(2);	// see sBNumber->setMaximum()
	connect(cBDigits, SIGNAL(currentIndexChanged(int)), this, SLOT(digitCBChanged(int)));

	lEText = new QLineEdit(this);
	connect(cBCase, SIGNAL(currentIndexChanged(int)), this, SIGNAL(changed()));
	connect(lEText, SIGNAL(textChanged(QString)), this, SIGNAL(changed()));

	pbPlus = new QPushButton("+", this);
	pbPlus->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	pbPlus->setMinimumSize(10,10);
	pbPlus->setMaximumSize(30,30);
	pbMinus = new QPushButton("-", this);
	pbMinus->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Minimum);
	pbMinus->setMinimumSize(10,10);
	pbMinus->setMaximumSize(30,30);
	connect(pbPlus, SIGNAL(clicked()), this, SLOT(pbPlusPressed()));
	connect(pbMinus, SIGNAL(clicked()), this, SLOT(pbMinusPressed()));
	connect(pbPlus, SIGNAL(clicked()), this, SIGNAL(changed()));
	connect(pbMinus, SIGNAL(clicked()), this, SIGNAL(changed()));
}

void DkFilenameWidget::typeCBChanged(int index) {
	switch (index) {
		case fileNameTypes_fileName: {showOnlyFilename(); break;};
		case fileNameTypes_Text: {showOnlyText(); break;};
		case fileNameTypes_Number: {showOnlyNumber(); break;};
		default:
			break;
	}
}

void DkFilenameWidget::showOnlyFilename() {
	cBCase->show();

	sBNumber->hide();
	cBDigits->hide();
	lEText->hide();

	curLayout->addWidget(cBType, 0, fileNameWidget_type);
	curLayout->addWidget(cBCase, 0, fileNameWidget_input1);
	//curLayout->addWidget(new QWidget(this), 0, fileNameWidget_input2 );
	curLayout->addWidget(pbPlus, 0, fileNameWidget_plus);
	curLayout->addWidget(pbMinus, 0, fileNameWidget_minus);

}

void DkFilenameWidget::showOnlyNumber() {
	sBNumber->show();
	cBDigits->show();

	cBCase->hide();
	lEText->hide();

	curLayout->addWidget(cBType, 0, fileNameWidget_type);
	curLayout->addWidget(sBNumber, 0, fileNameWidget_input1);
	curLayout->addWidget(cBDigits, 0, fileNameWidget_input2);
	curLayout->addWidget(pbPlus, 0, fileNameWidget_plus);
	curLayout->addWidget(pbMinus, 0, fileNameWidget_minus);
}

void DkFilenameWidget::showOnlyText() {
	lEText->show();

	sBNumber->hide();
	cBDigits->hide();
	cBCase->hide();
	

	curLayout->addWidget(cBType, 0, fileNameWidget_type);
	curLayout->addWidget(lEText, 0, fileNameWidget_input1);
	//curLayout->addWidget(new QWidget(this), 0, fileNameWidget_input2);
	curLayout->addWidget(pbPlus, 0, fileNameWidget_plus);
	curLayout->addWidget(pbMinus, 0, fileNameWidget_minus);
}

void DkFilenameWidget::pbPlusPressed() {
	emit plusPressed(this);
}

void DkFilenameWidget::pbMinusPressed() {
	emit minusPressed(this);
}

void DkFilenameWidget::enableMinusButton(bool enable) {
	pbMinus->setEnabled(enable);
}

void DkFilenameWidget::enablePlusButton(bool enable) {
	pbPlus->setEnabled(enable);
}

void DkFilenameWidget::checkForUserInput() {
	if(cBType->currentIndex() == 0 && cBCase->currentIndex() == 0)
		hasChanged = false;
	else
		hasChanged = true;
	//emit changed();
}

void DkFilenameWidget::digitCBChanged(int index) {
	sBNumber->setMaximum(qRound(std::pow(10, index+1)-1));
	emit changed();
}

QString DkFilenameWidget::getTag() const {

	QString tag;

	switch (cBType->currentIndex()) {
		
	case fileNameTypes_Number: 
		{
			tag += "<d:"; 
			tag += QString::number(cBDigits->currentIndex());	// is sensitive to the index
			tag += ":" + QString::number(sBNumber->value());
			tag += ">";
			break;
		}
	case fileNameTypes_fileName: 
		{
			tag += "<c:"; 
			tag += QString::number(cBCase->currentIndex());	// is sensitive to the index
			tag += ">";
			break;
		}
	case fileNameTypes_Text:
		{
			tag += lEText->text();
		}
	}

	return tag;
}

// DkBatchOutput --------------------------------------------------------------------
DkBatchOutput::DkBatchOutput(QWidget* parent , Qt::WindowFlags f ) : QWidget(parent, f) {
	this->rUserInput = false;
	setObjectName("DkBatchOutput");
	createLayout();

	outputDirectory = QDir();
}

void DkBatchOutput::createLayout() {

	// Output Directory Groupbox
	QGroupBox* outDirGroupBox = new QGroupBox(this);
	outDirGroupBox->setTitle(tr("Output Directory"));

	outputBrowseButton = new QPushButton(tr("Browse"));
	outputlineEdit = new DkDirectoryEdit(this);
	outputlineEdit->setPlaceholderText(tr("Select a Directory"));
	connect(outputBrowseButton , SIGNAL(clicked()), this, SLOT(browse()));
	connect(outputlineEdit, SIGNAL(textChanged(QString)), this, SLOT(outputTextChanged(QString)));

	// overwrite existing
	cbOverwriteExisting = new QCheckBox(tr("Overwrite Existing Files"));
	cbOverwriteExisting->setToolTip(tr("If checked, existing files are overwritten.\nThis option might destroy your images - so be careful!"));
	connect(cbOverwriteExisting, SIGNAL(clicked()), this, SIGNAL(changed()));

	// Use Input Folder
	cbUseInput = new QCheckBox(tr("Use Input Folder"));
	cbUseInput->setToolTip(tr("If checked, the batch is applied to the input folder - so be careful!"));
	connect(cbUseInput, SIGNAL(clicked(bool)), this, SLOT(useInputFolderChanged(bool)));

	// delete original
	cbDeleteOriginal = new QCheckBox(tr("Delete Input Files"));
	cbDeleteOriginal->setToolTip(tr("If checked, the original file will be deleted if the conversion was successful.\n So be careful!"));

	QWidget* cbWidget = new QWidget(this);
	QHBoxLayout* cbLayout = new QHBoxLayout(cbWidget);
	cbLayout->setContentsMargins(0,0,0,0);
	cbLayout->addWidget(cbUseInput);
	cbLayout->addWidget(cbOverwriteExisting);
	cbLayout->addWidget(cbDeleteOriginal);
	cbLayout->addStretch();

	QGridLayout* outDirLayout = new QGridLayout(outDirGroupBox);
	//outDirLayout->setContentsMargins(0, 0, 0, 0);
	outDirLayout->addWidget(outputBrowseButton, 0, 0);
	outDirLayout->addWidget(outputlineEdit, 0, 1);
	outDirLayout->addWidget(cbWidget, 1, 1);

	// Filename Groupbox
	QGroupBox* filenameGroupBox = new QGroupBox(this);
	filenameGroupBox->setTitle(tr("Filename"));
	filenameVBLayout = new QVBoxLayout(filenameGroupBox);
	filenameVBLayout->setSpacing(0);
	//filenameVBLayout->setContentsMargins(0,0,0,0);
	DkFilenameWidget* fwidget = new DkFilenameWidget(this);
	fwidget->enableMinusButton(false);
	filenameWidgets.push_back(fwidget);
	filenameVBLayout->addWidget(fwidget);
	connect(fwidget, SIGNAL(plusPressed(DkFilenameWidget*)), this, SLOT(plusPressed(DkFilenameWidget*)));
	connect(fwidget, SIGNAL(minusPressed(DkFilenameWidget*)), this, SLOT(minusPressed(DkFilenameWidget*)));
	connect(fwidget, SIGNAL(changed()), this, SLOT(emitChangedSignal()));

	QWidget* extensionWidget = new QWidget(this);
	QHBoxLayout* extensionLayout = new QHBoxLayout(extensionWidget);
	//extensionLayout->setSpacing(0);
	extensionLayout->setContentsMargins(0,0,0,0);
	cBExtension = new QComboBox(this);
	cBExtension->addItem(tr("Keep Extension"));
	cBExtension->addItem(tr("Convert To"));
	connect(cBExtension, SIGNAL(currentIndexChanged(int)), this, SLOT(extensionCBChanged(int)));

	cBNewExtension = new QComboBox(this);
	cBNewExtension->addItems(DkSettings::app.saveFilters);
	cBNewExtension->setEnabled(false);

	extensionLayout->addWidget(cBExtension);
	extensionLayout->addWidget(cBNewExtension);
	extensionLayout->addStretch();
	filenameVBLayout->addWidget(extensionWidget);
	
	QLabel* oldLabel = new QLabel(tr("Old: "));
	oldFileNameLabel = new QLabel("");

	QLabel* newLabel = new QLabel(tr("New: "));
	newFileNameLabel = new QLabel("");

	// Preview Widget
	QGroupBox* previewGroupBox = new QGroupBox(this);
	previewGroupBox->setTitle(tr("Filename Preview"));
	QGridLayout* previewGBLayout = new QGridLayout(previewGroupBox);
	//previewGroupBox->hide();	// show if more than 1 file is selected
	previewGBLayout->addWidget(oldLabel, 0, 0);
	previewGBLayout->addWidget(oldFileNameLabel, 0, 1);
	previewGBLayout->addWidget(newLabel, 1, 0);
	previewGBLayout->addWidget(newFileNameLabel, 1, 1);
	previewGBLayout->setColumnStretch(3, 10);
	previewGBLayout->setAlignment(Qt::AlignTop);
	
	QGridLayout* contentLayout = new QGridLayout(this);
	contentLayout->addWidget(outDirGroupBox, 0, 0, 1, 2);
	contentLayout->addWidget(filenameGroupBox, 1, 0);
	contentLayout->addWidget(previewGroupBox, 1, 1);
	setLayout(contentLayout);
}

void DkBatchOutput::browse() {

	QString dirGuess = (outputlineEdit->text().isEmpty()) ? inputDirectory.absolutePath() : outputlineEdit->text();
	
	// load system default open dialog
	QString dirName = QFileDialog::getExistingDirectory(this, tr("Open an Image Directory"),
		dirGuess);

	if (dirName.isEmpty())
		return;

	setDir(QDir(dirName));
}

void DkBatchOutput::setDir(QDir dir, bool updateLineEdit) {

	outputDirectory = dir;
	emit newHeaderText(dir.absolutePath());
	
	if (updateLineEdit)
		outputlineEdit->setText(dir.absolutePath());
}

void DkBatchOutput::setInputDir(QDir dir) {
	inputDirectory = dir;

	if (cbUseInput->isChecked())
		setDir(inputDirectory);
}

void DkBatchOutput::useInputFolderChanged(bool checked) {

	outputlineEdit->setEnabled(!checked);
	outputBrowseButton->setEnabled(!checked);

	if (checked)
		setDir(inputDirectory);
}

void DkBatchOutput::plusPressed(DkFilenameWidget* widget) {
	int index = filenameVBLayout->indexOf(widget);
	DkFilenameWidget* fwidget = new DkFilenameWidget(this);
	filenameWidgets.insert(index + 1, fwidget);
	if (filenameWidgets.size() > 4) {
		for (int i = 0; i  < filenameWidgets.size(); i++)
			filenameWidgets[i]->enablePlusButton(false);
	}
	filenameVBLayout->insertWidget(index + 1, fwidget); // append to current widget
	connect(fwidget, SIGNAL(plusPressed(DkFilenameWidget*)), this, SLOT(plusPressed(DkFilenameWidget*)));
	connect(fwidget, SIGNAL(minusPressed(DkFilenameWidget*)), this, SLOT(minusPressed(DkFilenameWidget*)));
	connect(fwidget, SIGNAL(changed()), this, SLOT(emitChangedSignal()));

	emitChangedSignal();
}

void DkBatchOutput::minusPressed(DkFilenameWidget* widget) {
	filenameVBLayout->removeWidget(widget);
	filenameWidgets.remove(filenameWidgets.indexOf(widget));
	if (filenameWidgets.size() <= 4) {
		for (int i = 0; i  < filenameWidgets.size(); i++)
			filenameWidgets[i]->enablePlusButton(true);
	}

	widget->hide();

	emitChangedSignal();
}

void DkBatchOutput::extensionCBChanged(int index) {
	//index > 0 ? cBNewExtension->show() : cBNewExtension->hide();
	cBNewExtension->setEnabled(index > 0);
	emitChangedSignal();
}


bool DkBatchOutput::hasUserInput() const {
	// TODO add output directory 
	return filenameWidgets.size() > 1 || filenameWidgets[0]->hasUserInput() || cBExtension->currentIndex() == 1;
}

void DkBatchOutput::emitChangedSignal() {

	updateFileLabelPreview();
	emit changed();
}

void DkBatchOutput::updateFileLabelPreview() {

	qDebug() << "updating file label, example name: " << exampleName;

	if (exampleName.isEmpty())
		return;

	DkFileNameConverter converter(exampleName, getFilePattern(), 0);

	oldFileNameLabel->setText(exampleName);
	newFileNameLabel->setText(converter.getConvertedFileName());
}

void DkBatchOutput::outputTextChanged(QString text) {

	setDir(QDir(text), false);
}

QString DkBatchOutput::getOutputDirectory() {
	qDebug() << "ouptut dir: " << QDir(outputlineEdit->text()).absolutePath();
	return outputlineEdit->text();
}

QString DkBatchOutput::getFilePattern() {

	QString pattern = "";

	for (int idx = 0; idx < filenameWidgets.size(); idx++)
		pattern += filenameWidgets.at(idx)->getTag();	

	if (cBExtension->currentIndex() == 0) {
		pattern += ".<old>";
	}
	else {
		QString ext = cBNewExtension->itemText(cBNewExtension->currentIndex());

		QStringList tmp = ext.split("(");

		if (tmp.size() == 2) {

			QString filters = tmp.at(1);
			filters.replace(")", "");
			filters.replace("*", "");

			QStringList extList = filters.split(" ");

			if (!extList.empty())
				pattern += extList[0];
		}
	}

	return pattern;
}

int DkBatchOutput::overwriteMode() const {

	if (cbOverwriteExisting->isChecked())
		return DkBatchConfig::mode_overwrite;

	return DkBatchConfig::mode_skip_existing;
}

bool DkBatchOutput::deleteOriginal() const {

	return cbDeleteOriginal->isChecked();
}

void DkBatchOutput::setExampleFilename(const QString& exampleName) {

	this->exampleName = exampleName;
	qDebug() << "example name: " << exampleName;
	updateFileLabelPreview();
}

// DkResizeWidget --------------------------------------------------------------------
DkBatchResizeWidget::DkBatchResizeWidget(QWidget* parent /* = 0 */, Qt::WindowFlags f /* = 0 */) : QWidget(parent, f) {

	createLayout();
	modeChanged(0);	// init gui
}

void DkBatchResizeWidget::createLayout() {

	comboMode = new QComboBox(this);
	QStringList modeItems;
	modeItems << tr("Percent") << tr("Long Side") << tr("Short Side") << tr("Width") << tr("Height");
	comboMode->addItems(modeItems);

	comboProperties = new QComboBox(this);
	QStringList propertyItems;
	propertyItems << tr("Transform All") << tr("Shrink Only") << tr("Enlarge Only");
	comboProperties->addItems(propertyItems);

	sbPercent = new QDoubleSpinBox(this);
	sbPercent->setSuffix(tr("%"));
	sbPercent->setMaximum(1000);
	sbPercent->setMinimum(0.1);
	sbPercent->setValue(100.0);

	sbPx = new QSpinBox(this);
	sbPx->setSuffix(tr(" px"));
	sbPx->setMaximum(SHRT_MAX);
	sbPx->setMinimum(1);
	sbPx->setValue(1920);

	QHBoxLayout* layout = new QHBoxLayout(this);
	layout->addWidget(comboMode);
	layout->addWidget(sbPercent);
	layout->addWidget(sbPx);
	layout->addWidget(comboProperties);
	layout->addStretch();

	connect(comboMode, SIGNAL(currentIndexChanged(int)), this, SLOT(modeChanged(int)));
	connect(sbPercent, SIGNAL(valueChanged(double)), this, SLOT(percentChanged(double)));
	connect(sbPx, SIGNAL(valueChanged(int)), this, SLOT(pxChanged(int)));
}

void DkBatchResizeWidget::modeChanged(int) {

	if (comboMode->currentIndex() == DkResizeBatch::mode_default) {
		sbPx->hide();
		sbPercent->show();
		comboProperties->hide();
		percentChanged(sbPercent->value());
	}
	else {
		sbPx->show();
		sbPercent->hide();
		comboProperties->show();
		pxChanged(sbPx->value());
	}
}

void DkBatchResizeWidget::percentChanged(double val) {

	if (val == 100.0)
		emit newHeaderText(tr("inactive"));
	else
		emit newHeaderText(QString::number(val) + "%");
}

void DkBatchResizeWidget::pxChanged(int val) {

	emit newHeaderText(comboMode->itemText(comboMode->currentIndex()) + ": " + QString::number(val) + " px");
}

void DkBatchResizeWidget::transferProperties(QSharedPointer<DkResizeBatch> batchResize) const {

	if (comboMode->currentIndex() == DkResizeBatch::mode_default) {
		batchResize->setProperties((float)sbPercent->value()/100.0f, comboMode->currentIndex());
	}
	else {
		batchResize->setProperties((float)sbPx->value(), comboMode->currentIndex(), comboProperties->currentIndex());
	}
}

bool DkBatchResizeWidget::hasUserInput() const {

	return !(comboMode->currentIndex() == DkResizeBatch::mode_default && sbPercent->value() == 100.0);
}

bool DkBatchResizeWidget::requiresUserInput() const {

	return false;
}

// DkBatchTransform --------------------------------------------------------------------
DkBatchTransformWidget::DkBatchTransformWidget(QWidget* parent /* = 0 */, Qt::WindowFlags f /* = 0 */) : QWidget(parent, f) {

	createLayout();
}

void DkBatchTransformWidget::createLayout() {

	rbRotate0 = new QRadioButton(tr("Do &Not Rotate"));
	rbRotate0->setChecked(true);
	rbRotateLeft = new QRadioButton(tr("9&0\u00B0 Counter Clockwise"));
	rbRotateRight = new QRadioButton(tr("&90\u00B0 Clockwise"));
	rbRotate180 = new QRadioButton(tr("&180\u00B0"));

	rotateGroup = new QButtonGroup(this);

	rotateGroup->addButton(rbRotate0);
	rotateGroup->addButton(rbRotateLeft);
	rotateGroup->addButton(rbRotateRight);
	rotateGroup->addButton(rbRotate180);

	cbFlipH = new QCheckBox(tr("Flip &Horizontal"));
	cbFlipV = new QCheckBox(tr("Flip &Vertical"));

	QGridLayout* layout = new QGridLayout(this);
	layout->addWidget(rbRotate0, 0, 0);
	layout->addWidget(rbRotateRight, 1, 0);
	layout->addWidget(rbRotateLeft, 2, 0);
	layout->addWidget(rbRotate180, 3, 0);

	layout->addWidget(cbFlipH, 0, 1);
	layout->addWidget(cbFlipV, 1, 1);
	layout->setColumnStretch(3, 10);

	connect(rotateGroup, SIGNAL(buttonClicked(int)), this, SLOT(radioButtonClicked(int)));
	connect(cbFlipV, SIGNAL(clicked()), this, SLOT(checkBoxClicked()));
	connect(cbFlipH, SIGNAL(clicked()), this, SLOT(checkBoxClicked()));
}

bool DkBatchTransformWidget::hasUserInput() const {
	
	return !rbRotate0->isChecked() || cbFlipH->isChecked() || cbFlipV->isChecked();
}

bool DkBatchTransformWidget::requiresUserInput() const {

	return false;
}

void DkBatchTransformWidget::radioButtonClicked(int) {

	updateHeader();
}

void DkBatchTransformWidget::checkBoxClicked() {

	updateHeader();
}

void DkBatchTransformWidget::updateHeader() const {

	if (!hasUserInput())
		emit newHeaderText(tr("inactive"));
	else {
		
		QString txt;
		if (getAngle() != 0)
			txt += tr("Rotating by: %1").arg(getAngle());
		if (cbFlipH->isChecked() || cbFlipV->isChecked()) {
			txt += tr(" Flipping");
		}
		
		emit newHeaderText(txt);
	}
}

void DkBatchTransformWidget::transferProperties(QSharedPointer<DkBatchTransform> batchTransform) const {

	batchTransform->setProperties(getAngle(), cbFlipH->isChecked(), cbFlipV->isChecked());
}

int DkBatchTransformWidget::getAngle() const {

	if (rbRotate0->isChecked())
		return 0;
	else if (rbRotateLeft->isChecked())
		return -90;
	else if (rbRotateRight->isChecked())
		return 90;
	else if (rbRotate180->isChecked())
		return 180;

	return 0;
}

// Batch Dialog --------------------------------------------------------------------
DkBatchDialog::DkBatchDialog(QDir currentDirectory, QWidget* parent /* = 0 */, Qt::WindowFlags f /* = 0 */) : QDialog(parent, f) {
	
	this->currentDirectory = currentDirectory;
	batchProcessing = new DkBatchProcessing(DkBatchConfig(), this);

	connect(batchProcessing, SIGNAL(progressValueChanged(int)), this, SLOT(updateProgress(int)));
	connect(batchProcessing, SIGNAL(finished()), this, SLOT(processingFinished()));

	setWindowTitle(tr("Batch Conversion"));
	createLayout();
	connect(fileSelection, SIGNAL(updateInputDir(QDir)), outputSelection, SLOT(setInputDir(QDir)));
	
	connect(&logUpdateTimer, SIGNAL(timeout()), this, SLOT(updateLog()));

	fileSelection->setDir(currentDirectory);
	outputSelection->setInputDir(currentDirectory);
	//outputSelection->setDir(currentDirectory);
}

void DkBatchDialog::createLayout() {

	//setStyleSheet("QWidget{border: 1px solid #000000;}");

	widgets.resize(batchWidgets_end);
	// Input Directory
	widgets[batch_input] = new DkBatchWidget(tr("Input Directory"), tr("directory not set"), this);
	fileSelection  = new DkFileSelection(widgets[batch_input]);
	widgets[batch_input]->setContentWidget(fileSelection);
	fileSelection->setDir(currentDirectory);
	
	// fold content
	widgets[batch_resize] = new DkBatchWidget(tr("Resize"), tr("inactive"), this);
	resizeWidget = new DkBatchResizeWidget(widgets[batch_resize]);
	widgets[batch_resize]->setContentWidget(resizeWidget);
	widgets[batch_resize]->showContent(false);

	widgets[batch_transform] = new DkBatchWidget(tr("Transform"), tr("inactive"), this);
	transformWidget = new DkBatchTransformWidget(widgets[batch_transform]);
	widgets[batch_transform]->setContentWidget(transformWidget);
	widgets[batch_transform]->showContent(false);

	widgets[batch_output] = new DkBatchWidget(tr("Output"), tr("not set"), this);
	outputSelection = new DkBatchOutput(widgets[batch_output]);
	widgets[batch_output]->setContentWidget(outputSelection);

	progressBar = new QProgressBar(this);
	progressBar->setVisible(false);

	summaryLabel = new QLabel("", this);
	summaryLabel->setObjectName("DkDecentInfo");
	summaryLabel->setVisible(false);
	summaryLabel->setAlignment(Qt::AlignRight);

	// buttons
	logButton = new QPushButton(tr("Show &Log"), this);
	logButton->setToolTip(tr("Shows detailed status messages."));
	logButton->setEnabled(false);
	connect(logButton, SIGNAL(clicked()), this, SLOT(logButtonClicked()));

	buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, Qt::Horizontal);
	buttons->button(QDialogButtonBox::Ok)->setDefault(true);	// ok is auto-default
	buttons->button(QDialogButtonBox::Ok)->setText(tr("&OK"));
	buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
	buttons->button(QDialogButtonBox::Cancel)->setText(tr("&Close"));
	buttons->addButton(logButton, QDialogButtonBox::ActionRole);

	connect(buttons, SIGNAL(accepted()), this, SLOT(accept()));
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));

	QVBoxLayout* dialogLayout = new QVBoxLayout(this);
	dialogLayout->setAlignment(Qt::AlignTop);
	for (int i=0; i < widgets.size(); i++) {
		dialogLayout->addWidget(widgets[i]);
		//connect(widgets[i]->contentWidget(), SIGNAL(changed()), this, SLOT(widgetChanged())); // most widgets do not have (and need) a changed signal ... perhaps uncomment this again 
	}
	connect(widgets[batch_input]->contentWidget(), SIGNAL(changed()), this, SLOT(widgetChanged()));
	connect(widgets[batch_output]->contentWidget(), SIGNAL(changed()), this, SLOT(widgetChanged())); 

	dialogLayout->addWidget(progressBar);
	dialogLayout->addWidget(summaryLabel);
	//dialogLayout->addStretch(10);
	dialogLayout->addWidget(buttons);

	setLayout(dialogLayout);
}

void DkBatchDialog::accept() {
	
	// check if we are good to go
	if (fileSelection->getSelectedFiles().empty()) {
		QMessageBox::information(this, tr("Wrong Configuration"), tr("Please select files for processing."), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	DkBatchOutput* outputWidget = dynamic_cast<DkBatchOutput*>(widgets[batch_output]->contentWidget());

	if (!outputWidget) {
		qDebug() << "FATAL ERROR: could not cast output widget";
		QMessageBox::critical(this, tr("Fatal Error"), tr("I am missing a widget."), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	if (widgets[batch_output] && widgets[batch_input])  {
		bool outputChanged = dynamic_cast<DkBatchContent*>(widgets[batch_output]->contentWidget())->hasUserInput();
		QString inputDirPath = dynamic_cast<DkFileSelection*>(widgets[batch_input]->contentWidget())->getDir();
		QString outputDirPath = dynamic_cast<DkBatchOutput*>(widgets[batch_output]->contentWidget())->getOutputDirectory();
		
		if (!outputChanged && inputDirPath.toLower() == outputDirPath.toLower() && dynamic_cast<DkBatchOutput*>(widgets[batch_output]->contentWidget())->overwriteMode() != DkBatchConfig::mode_overwrite) {
			QMessageBox::information(this, tr("Wrong Configuration"), tr("Please check 'Overwrite Existing Files' or choose a different output directory."), QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
	}

	DkBatchConfig config(fileSelection->getSelectedFilesBatch(), outputWidget->getOutputDirectory(), outputWidget->getFilePattern());
	config.setMode(outputWidget->overwriteMode());
	config.setDeleteOriginal(outputWidget->deleteOriginal());

	if (!config.getOutputDirPath().isEmpty() && !QDir(config.getOutputDirPath()).exists()) {

		DkMessageBox* msgBox = new DkMessageBox(QMessageBox::Question, tr("Create Output Directory"), 
			tr("Should I create:\n%1").arg(config.getOutputDirPath()), 
			(QMessageBox::Yes | QMessageBox::No | QMessageBox::Cancel), qApp->activeWindow());

		msgBox->setDefaultButton(QMessageBox::Yes);
		msgBox->setObjectName("batchOutputDirDialog");

		int answer = msgBox->exec();

		if (answer != QMessageBox::Accepted && answer != QMessageBox::Yes) {
			return;
		}
	}

	if (!config.isOk()) {

		if (config.getOutputDirPath().isEmpty()) {
			QMessageBox::critical(this, tr("Fatal Error"), tr("Please select an output directory."), QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		else if (!QDir(config.getOutputDirPath()).exists()) {
			QMessageBox::critical(this, tr("Fatal Error"), tr("Sorry, I cannot create %1.").arg(config.getOutputDirPath()), QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		else if (config.getFileList().empty()) {
			QMessageBox::critical(this, tr("Fatal Error"), tr("Sorry, I cannot find files to process."), QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		else if (config.getFileNamePattern().isEmpty()) {
			QMessageBox::critical(this, tr("Fatal Error"), tr("Sorry, the file pattern is empty."), QMessageBox::Ok, QMessageBox::Ok);
			return;
		}
		//else if (config.getOutputDir() == QDir()) {
		//	QMessageBox::information(this, tr("Input Missing"), tr("Please choose a valid output directory\n%1").arg(config.getOutputDir().absolutePath()), QMessageBox::Ok, QMessageBox::Ok);
		//	return;
		//}

		qDebug() << "config not ok - canceling";
		QMessageBox::critical(this, tr("Fatal Error"), tr("Sorry, the file pattern is empty."), QMessageBox::Ok, QMessageBox::Ok);
		return;
	}

	// create processing functions
	QSharedPointer<DkResizeBatch> resizeBatch(new DkResizeBatch);
	resizeWidget->transferProperties(resizeBatch);

	// create processing functions
	QSharedPointer<DkBatchTransform> transformBatch(new DkBatchTransform);
	transformWidget->transferProperties(transformBatch);

	QVector<QSharedPointer<DkAbstractBatch> > processFunctions;
	
	if (resizeBatch->isActive())
		processFunctions.append(resizeBatch);

	if (transformBatch->isActive())
		processFunctions.append(transformBatch);

	config.setProcessFunctions(processFunctions);
	batchProcessing->setBatchConfig(config);

	startProcessing();
	batchProcessing->compute();
}

void DkBatchDialog::reject() {

	if (batchProcessing->isComputing()) {
		batchProcessing->cancel();
		buttons->button(QDialogButtonBox::Cancel)->setEnabled(false);
		//stopProcessing();
	}
	else
		QDialog::reject();
}

void DkBatchDialog::processingFinished() {

	stopProcessing();
}

void DkBatchDialog::startProcessing() {

	fileSelection->startProcessing();

	progressBar->show();
	progressBar->reset();
	progressBar->setMaximum(fileSelection->getSelectedFiles().size());
	logButton->setEnabled(false);
	buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
	buttons->button(QDialogButtonBox::Cancel)->setText(tr("&Cancel"));

	logNeedsUpdate = false;
	logUpdateTimer.start(1000);
}

void DkBatchDialog::stopProcessing() {

	fileSelection->stopProcessing();

	progressBar->hide();
	progressBar->reset();
	logButton->setEnabled(true);
	buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
	buttons->button(QDialogButtonBox::Cancel)->setEnabled(true);
	buttons->button(QDialogButtonBox::Cancel)->setText(tr("&Close"));

	int numFailures = batchProcessing->getNumFailures();
	int numProcessed = batchProcessing->getNumProcessed();
	int numItems = batchProcessing->getNumItems();

	summaryLabel->setText(tr("%1/%2 files processed... %3 failed.").arg(numProcessed).arg(numItems).arg(numFailures));
	summaryLabel->show();

	summaryLabel->setProperty("warning", numFailures > 0);
	summaryLabel->style()->unpolish(this);
	summaryLabel->style()->polish(this);
	update();

	logNeedsUpdate = false;
	logUpdateTimer.stop();

	updateLog();
}

void DkBatchDialog::updateLog() {

	fileSelection->setResults(batchProcessing->getResultList());
}

void DkBatchDialog::updateProgress(int progress) {

	progressBar->setValue(progress);
	logNeedsUpdate = true;
}

void DkBatchDialog::logButtonClicked() {

	QStringList log = batchProcessing->getLog();

	DkTextDialog* textDialog = new DkTextDialog(this);
	textDialog->getTextEdit()->setReadOnly(true);
	textDialog->setText(log);

	textDialog->exec();
}

void DkBatchDialog::setSelectedFiles(const QStringList& selFiles) {

	if (!selFiles.empty()) {
		fileSelection->getInputEdit()->appendFiles(selFiles);
		fileSelection->changeTab(DkFileSelection::tab_text_input);
	}

}

void DkBatchDialog::widgetChanged() {
	
	if (widgets[batch_output] && widgets[batch_input])  {
		QString inputDirPath = dynamic_cast<DkFileSelection*>(widgets[batch_input]->contentWidget())->getDir();
		QString outputDirPath = dynamic_cast<DkBatchOutput*>(widgets[batch_output]->contentWidget())->getOutputDirectory();
		
		if (inputDirPath == "" || outputDirPath == "")
			buttons->button(QDialogButtonBox::Ok)->setEnabled(false);
		else
			buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
	}

	if (!fileSelection->getSelectedFiles().isEmpty()) {

		QUrl url = fileSelection->getSelectedFiles().first();
		QString fString = url.toString();
		fString = fString.replace("file:///", "");

		QFileInfo cFileInfo = QFileInfo(fString);
		if (!cFileInfo.exists())	// try an alternative conversion
			cFileInfo = QFileInfo(url.toLocalFile());

		dynamic_cast<DkBatchOutput*>(widgets[batch_output]->contentWidget())->setExampleFilename(cFileInfo.fileName());
		buttons->button(QDialogButtonBox::Ok)->setEnabled(true);
	}
}

}
