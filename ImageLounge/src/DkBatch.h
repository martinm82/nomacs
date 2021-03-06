/*******************************************************************************************************
DkNoMacs.h
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
#pragma once

#pragma warning(push, 0)	// no warnings from includes - begin
#include <QWidget>
#include <QUrl>
#include <QDir>
#include <QDialog>
#include <QTextEdit>
#pragma warning(pop)		// no warnings from includes - end

#include "DkImageContainer.h"

// Qt defines
class QListView;
class QVBoxLayout;
class QLabel;
class QFileInfo;
class QComboBox;
class QLineEdit;
class QSpinBox;
class QDoubleSpinBox;
class QPushButton;
class QGridLayout;
class QCheckBox;
class QButtonGroup;
class QRadioButton;
class QDialogButtonBox;
class QProgressBar;
class QTabWidget;

namespace nmc {

// TODO: these enums are global - they should be put into the respective classes
enum fileNameTypes {
fileNameTypes_fileName,
fileNameTypes_Number,
fileNameTypes_Text,

fileNameTypes_end
};

enum fileNameWidget {
fileNameWidget_type,
fileNameWidget_input1,
fileNameWidget_input2,
fileNameWidget_plus,
fileNameWidget_minus,

fileNameWidget_end
};

// nomacs defines
class DkResizeBatch;
class DkBatchProcessing;
class DkBatchTransform;
class DkBatchContent;
class DkButton;
class DkThumbScrollWidget;
class DkImageLoader;
class DkExplorer;
class DkDirectoryEdit;

class DkBatchContent {
public:
	virtual bool hasUserInput() const = 0;
	virtual bool requiresUserInput() const = 0;
};

class DkBatchWidget : public QWidget {
	Q_OBJECT

public:
	DkBatchWidget(QString titleString, QString headerString, QWidget* parent = 0, Qt::WindowFlags f = 0);
	
	void setContentWidget(QWidget* batchContent);
	QWidget* contentWidget() const;

public slots:
	void setTitle(QString title);
	void setHeader(QString header);
	void showContent(bool show);

protected:
	virtual void createLayout();

private:
	DkBatchContent* batchContent;
	QVBoxLayout* batchWidgetLayout;
	QString titleString, headerString;
	QLabel* titleLabel; 
	QLabel* headerLabel;
	DkButton* showButton;
};

class DkInputTextEdit : public QTextEdit {
	Q_OBJECT

public:
	DkInputTextEdit(QWidget* parent = 0);

	QStringList getFileList() const;
	void appendDir(const QDir& newDir, bool recursive = false);
	void insertFromMimeData(const QMimeData *src);
	void clear();

signals:
	void fileListChangedSignal() const;

public slots:
	void appendFiles(const QStringList& fileList);

protected:
	void dropEvent(QDropEvent *event);
	void dragEnterEvent(QDragEnterEvent *event);
	void dragMoveEvent(QDragMoveEvent *event);
	void appendFromMime(const QMimeData* mimeData);

	QList<int> resultList;
};

class DkFileSelection : public QWidget, public DkBatchContent  {
	Q_OBJECT

public:

	enum {
		tab_thumbs = 0,
		tab_text_input,
		tab_results,

		tab_end
	};

	DkFileSelection(QWidget* parent = 0, Qt::WindowFlags f = 0);

	QString getDir() const;
	QStringList getSelectedFiles() const;
	QStringList getSelectedFilesBatch();
	DkInputTextEdit* getInputEdit() const;

	virtual bool hasUserInput() const {return hUserInput;};
	virtual bool requiresUserInput() const {return rUserInput;};
	void changeTab(int tabIdx) const;
	void startProcessing();
	void stopProcessing();
	void setResults(const QStringList& results);


public slots:
	void setDir(QDir dir);
	void browse();
	void updateDir(QVector<QSharedPointer<DkImageContainerT> >);
	void setVisible(bool visible);
	void emitChangedSignal();
	void selectionChanged();
	void setFileInfo(QFileInfo file);

signals:
	void updateDirSignal(QVector<QSharedPointer<DkImageContainerT> >);
	void newHeaderText(QString);
	void updateInputDir(QDir);
	void changed();

protected:
	virtual void createLayout();

	QDir cDir;
	QListView* fileWidget;
	DkThumbScrollWidget* thumbScrollWidget;
	DkInputTextEdit* inputTextEdit;
	QTextEdit* resultTextEdit;
	QSharedPointer<DkImageLoader> loader;
	DkExplorer* explorer;
	DkDirectoryEdit* directoryEdit;
	QLabel* infoLabel;
	QTabWidget* inputTabs;

private:
	bool hUserInput;
	bool rUserInput;

};

class DkFilenameWidget : public QWidget {
Q_OBJECT

public:	
	DkFilenameWidget(QWidget* parent = 0);
	void enableMinusButton(bool enable);
	void enablePlusButton(bool enable);
	bool hasUserInput() const {return hasChanged;};
	QString getTag() const;

signals:
	void plusPressed(DkFilenameWidget*);
	void minusPressed(DkFilenameWidget*);
	void changed();

private slots:
	void typeCBChanged(int index);
	void pbPlusPressed();
	void pbMinusPressed();
	void checkForUserInput();
	void digitCBChanged(int index);

private:
	void createLayout();
	void showOnlyText();
	void showOnlyNumber();
	void showOnlyFilename();

	QComboBox* cBType;
		
	QLineEdit* lEText;
	QComboBox* cBCase;

	QSpinBox* sBNumber;
	QComboBox* cBDigits;
		
	QPushButton* pbPlus;
	QPushButton* pbMinus;

	QGridLayout* curLayout;

	bool hasChanged;
};

class DkBatchOutput : public QWidget, public DkBatchContent {
Q_OBJECT

public:
	DkBatchOutput(QWidget* parent = 0, Qt::WindowFlags f = 0);

	virtual bool hasUserInput() const;
	virtual bool requiresUserInput() const {return rUserInput;};
	int overwriteMode() const;
	bool deleteOriginal() const;
	QString getOutputDirectory();
	QString getFilePattern();
	void setExampleFilename(const QString& exampleName);
	void setDir(QDir dir, bool updateLineEdit = true);

signals:
	void newHeaderText(QString);
	void changed();

public slots:
	void setInputDir(QDir dir);

protected slots:
	void browse();
	void plusPressed(DkFilenameWidget* widget);
	void minusPressed(DkFilenameWidget* widget);
	void extensionCBChanged(int index);
	void emitChangedSignal();
	void updateFileLabelPreview();
	void outputTextChanged(QString text);
	void useInputFolderChanged(bool checked);

protected:
	virtual void createLayout();

private:

	bool hUserInput;
	bool rUserInput;
	QDir outputDirectory;
	QDir inputDirectory;
	DkDirectoryEdit* outputlineEdit;
	QVector<DkFilenameWidget*> filenameWidgets;
	QVBoxLayout* filenameVBLayout;
	QCheckBox* cbOverwriteExisting;
	QCheckBox* cbUseInput;
	QCheckBox* cbDeleteOriginal;
	QPushButton* outputBrowseButton;

	QComboBox* cBExtension;
	QComboBox* cBNewExtension;
	QLabel* oldFileNameLabel;
	QLabel* newFileNameLabel;
	QString exampleName;

};

class DkBatchResizeWidget : public QWidget, public DkBatchContent {
	Q_OBJECT

public:
	DkBatchResizeWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);

	void transferProperties(QSharedPointer<DkResizeBatch> batchResize) const;
	bool hasUserInput() const;
	bool requiresUserInput() const;

public slots:
	void modeChanged(int idx);
	void percentChanged(double val);
	void pxChanged(int val);

signals:
	void newHeaderText(QString txt);

protected:
	void createLayout();

	QComboBox* comboMode;
	QComboBox* comboProperties;
	QSpinBox* sbPx;
	QDoubleSpinBox* sbPercent;
};

class DkBatchTransformWidget : public QWidget, public DkBatchContent {
	Q_OBJECT

public:
	DkBatchTransformWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);

	void transferProperties(QSharedPointer<DkBatchTransform> batchTransform) const;
	bool hasUserInput() const;
	bool requiresUserInput() const;

public slots:
	void radioButtonClicked(int id);
	void checkBoxClicked();

signals:
	void newHeaderText(QString txt) const;

protected:
	void createLayout();
	void updateHeader() const;
	int getAngle() const;

	QButtonGroup* rotateGroup;
	QRadioButton* rbRotate0;
	QRadioButton* rbRotateLeft;
	QRadioButton* rbRotateRight;
	QRadioButton* rbRotate180;

	QCheckBox* cbFlipH;
	QCheckBox* cbFlipV;
};

class DkBatchDialog : public QDialog {
	Q_OBJECT

public:
	DkBatchDialog(QDir currentDirectory = QDir(), QWidget* parent = 0, Qt::WindowFlags f = 0);

	enum batchWidgets {
		batch_input,
		batch_resize,
		batch_transform,
		batch_output,

		batchWidgets_end
	};

public slots:
	virtual void accept();
	virtual void reject();
	void widgetChanged();
	void logButtonClicked();
	void processingFinished();
	void updateProgress(int progress);
	void updateLog();
	void setSelectedFiles(const QStringList& selFiles);

protected:
	void createLayout();
		
private:
	QVector<DkBatchWidget*> widgets;
		
	QDir currentDirectory;
	QDialogButtonBox* buttons;
	DkFileSelection* fileSelection;
	DkBatchOutput* outputSelection;
	DkBatchResizeWidget* resizeWidget;
	DkBatchTransformWidget* transformWidget;
	DkBatchProcessing* batchProcessing;
	QPushButton* logButton;
	QProgressBar* progressBar;
	QLabel* summaryLabel;
	QTimer logUpdateTimer;
	bool logNeedsUpdate;

	void startProcessing();
	void stopProcessing();
};

}
