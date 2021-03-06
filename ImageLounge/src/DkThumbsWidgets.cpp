/*******************************************************************************************************
 DkThumbsWidgets.cpp
 Created on:	18.09.2014
 
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

#include "DkThumbsWidgets.h"
#include "DkThumbs.h"
#include "DkTimer.h"
#include "DkImageContainer.h"
#include "DkImageStorage.h"
#include "DkSettings.h"
#include "DkImage.h"

#pragma warning(push, 0)	// no warnings from includes - begin
#include <QTimer>
#include <QAction>
#include <QMenu>
#include <qmath.h>
#include <QResizeEvent>
#include <QGraphicsSceneMouseEvent>
#include <QApplication>
#include <QUrl>
#include <QScrollBar>
#include <QHBoxLayout>
#include <QStyleOptionGraphicsItem>
#include <QToolBar>
#include <QToolButton>
#include <QLineEdit>
#include <QClipboard>
#include <QMessageBox>
#include <QInputDialog>
#include <QMimeData>
#pragma warning(pop)		// no warnings from includes - end

namespace nmc {

// DkFilePreview --------------------------------------------------------------------
DkFilePreview::DkFilePreview(QWidget* parent, Qt::WindowFlags flags) : DkWidget(parent, flags) {

	this->parent = parent;
	orientation = Qt::Horizontal;
	windowPosition = pos_north;

	init();
	//setStyleSheet("QToolTip{border: 0px; border-radius: 21px; color: white; background-color: red;}"); //" + DkUtils::colorToString(bgCol) + ";}");

	loadSettings();
	initOrientations();

	createContextMenu();
}

void DkFilePreview::init() {

	setObjectName("DkFilePreview");
	setMouseTracking(true);	//receive mouse event everytime
	
	//thumbsLoader = 0;

	xOffset = qRound(DkSettings::display.thumbSize*0.1f);
	yOffset = qRound(DkSettings::display.thumbSize*0.1f);

	qDebug() << "x offset: " << xOffset;

	currentDx = 0;
	currentFileIdx = -1;
	oldFileIdx = -1;
	mouseTrace = 0;
	scrollToCurrentImage = false;
	isPainted = false;

	winPercent = 0.1f;
	borderTrigger = (orientation == Qt::Horizontal) ? (float)width()*winPercent : (float)height()*winPercent;
	//fileLabel = new DkGradientLabel(this);

	worldMatrix = QTransform();

	moveImageTimer = new QTimer(this);
	moveImageTimer->setInterval(5);	// reduce cpu utilization
	connect(moveImageTimer, SIGNAL(timeout()), this, SLOT(moveImages()));

	int borderTriggerI = qRound(borderTrigger);
	leftGradient = (orientation == Qt::Horizontal) ? QLinearGradient(QPoint(0, 0), QPoint(borderTriggerI, 0)) : QLinearGradient(QPoint(0, 0), QPoint(0, borderTriggerI));
	rightGradient = (orientation == Qt::Horizontal) ? QLinearGradient(QPoint(width()-borderTriggerI, 0), QPoint(width(), 0)) : QLinearGradient(QPoint(0, height()-borderTriggerI), QPoint(0, height()));
	leftGradient.setColorAt(1, Qt::white);
	leftGradient.setColorAt(0, Qt::black);
	rightGradient.setColorAt(1, Qt::black);
	rightGradient.setColorAt(0, Qt::white);

	minHeight = DkSettings::display.thumbSize + yOffset;
	//resize(parent->width(), minHeight);
	
	selected = -1;

	// wheel label
	QPixmap wp = QPixmap(":/nomacs/img/thumbs-move.png");
	wheelButton = new QLabel(this);
	wheelButton->setAttribute(Qt::WA_TransparentForMouseEvents);
	wheelButton->setPixmap(wp);
	wheelButton->hide();
}

void DkFilePreview::initOrientations() {

	if (windowPosition == pos_north || windowPosition == pos_south || windowPosition == pos_dock_hor)
		orientation = Qt::Horizontal;
	else if (windowPosition == pos_east || windowPosition == pos_west || windowPosition == pos_dock_ver)
		orientation = Qt::Vertical;

	if (windowPosition == pos_dock_ver || windowPosition == pos_dock_hor)
		minHeight = max_thumb_size;
	else
		minHeight = DkSettings::display.thumbSize;

	if (orientation == Qt::Horizontal) {

		setMinimumSize(20, 20);
		setMaximumSize(QWIDGETSIZE_MAX, minHeight);
		setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Expanding);
		borderTrigger = (float)width()*winPercent;
		int borderTriggerI = qRound(borderTrigger);
		leftGradient = QLinearGradient(QPoint(0, 0), QPoint(borderTriggerI, 0));
		rightGradient = QLinearGradient(QPoint(width()-borderTriggerI, 0), QPoint(width(), 0));
	}
	else {

		setMinimumSize(20, 20);
		setMaximumSize(minHeight, QWIDGETSIZE_MAX);
		setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Minimum);
		borderTrigger = (float)height()*winPercent;
		int borderTriggerI = qRound(borderTrigger);
		leftGradient = QLinearGradient(QPoint(0, 0), QPoint(0, borderTriggerI));
		rightGradient = QLinearGradient(QPoint(0, height()-borderTriggerI), QPoint(0, height()));
	}

	leftGradient.setColorAt(1, Qt::white);
	leftGradient.setColorAt(0, Qt::black);
	rightGradient.setColorAt(1, Qt::black);
	rightGradient.setColorAt(0, Qt::white);

	worldMatrix.reset();
	currentDx = 0;
	scrollToCurrentImage = true;
	update();

}

void DkFilePreview::loadSettings() {

	QSettings& settings = Settings::instance().getSettings();
	settings.beginGroup(objectName());
	windowPosition = settings.value("windowPosition", windowPosition).toInt();
	settings.endGroup();

}

void DkFilePreview::saveSettings() {

	QSettings& settings = Settings::instance().getSettings();
	settings.beginGroup(objectName());
	settings.setValue("windowPosition", windowPosition);
	settings.endGroup();
}

void DkFilePreview::createContextMenu() {

	contextMenuActions.resize(cm_end - 1);	// -1 because we just need to know of one dock widget

	contextMenuActions[cm_pos_west] = new QAction(tr("Show Left"), this);
	contextMenuActions[cm_pos_west]->setStatusTip(tr("Shows the Thumbnail Bar on the Left"));
	connect(contextMenuActions[cm_pos_west], SIGNAL(triggered()), this, SLOT(newPosition()));

	contextMenuActions[cm_pos_north] = new QAction(tr("Show Top"), this);
	contextMenuActions[cm_pos_north]->setStatusTip(tr("Shows the Thumbnail Bar at the Top"));
	connect(contextMenuActions[cm_pos_north], SIGNAL(triggered()), this, SLOT(newPosition()));

	contextMenuActions[cm_pos_east] = new QAction(tr("Show Right"), this);
	contextMenuActions[cm_pos_east]->setStatusTip(tr("Shows the Thumbnail Bar on the Right"));
	connect(contextMenuActions[cm_pos_east], SIGNAL(triggered()), this, SLOT(newPosition()));

	contextMenuActions[cm_pos_south] = new QAction(tr("Show Bottom"), this);
	contextMenuActions[cm_pos_south]->setStatusTip(tr("Shows the Thumbnail Bar at the Bottom"));
	connect(contextMenuActions[cm_pos_south], SIGNAL(triggered()), this, SLOT(newPosition()));

	contextMenuActions[cm_pos_dock_hor] = new QAction(tr("Undock"), this);
	contextMenuActions[cm_pos_dock_hor]->setStatusTip(tr("Undock the thumbnails"));
	connect(contextMenuActions[cm_pos_dock_hor], SIGNAL(triggered()), this, SLOT(newPosition()));

	contextMenu = new QMenu(tr("File Preview Menu"), this);
	contextMenu->addActions(contextMenuActions.toList());
}



void DkFilePreview::paintEvent(QPaintEvent*) {

	//if (selected != -1)
	//	resize(parent->width(), minHeight+fileLabel->height());	// catch parent resize...

	if (minHeight != DkSettings::display.thumbSize + yOffset && windowPosition != pos_dock_hor && windowPosition != pos_dock_ver) {

		xOffset = qCeil(DkSettings::display.thumbSize*0.1f);
		yOffset = qCeil(DkSettings::display.thumbSize*0.1f);

		minHeight = DkSettings::display.thumbSize + yOffset;
		
		if (orientation == Qt::Horizontal)
			setMaximumSize(QWIDGETSIZE_MAX, minHeight);
		else
			setMaximumSize(minHeight, QWIDGETSIZE_MAX);

		//if (fileLabel->height() >= height() && fileLabel->isVisible())
		//	fileLabel->hide();

	}
	//minHeight = DkSettings::DisplaySettings::thumbSize + yOffset;
	//resize(parent->width(), minHeight);

	//qDebug() << "size: " << size();

	QPainter painter(this);
	painter.setBackground(bgCol);

	painter.setPen(Qt::NoPen);
	painter.setBrush(bgCol);
	
	if (windowPosition != pos_dock_hor && windowPosition != pos_dock_ver) {
		QRect r = QRect(QPoint(), this->size());
		painter.drawRect(r);
	}

	painter.setWorldTransform(worldMatrix);
	painter.setWorldMatrixEnabled(true);

	if (thumbs.empty()) {
		thumbRects.clear();
		return;
	}

	painter.setRenderHint(QPainter::SmoothPixmapTransform);
	drawThumbs(&painter);

	if (currentFileIdx != oldFileIdx && currentFileIdx >= 0) {
		oldFileIdx = currentFileIdx;
		scrollToCurrentImage = true;
		moveImageTimer->start();
	}
	isPainted = true;

}

void DkFilePreview::drawThumbs(QPainter* painter) {

	//qDebug() << "drawing thumbs: " << worldMatrix.dx();

	bufferDim = (orientation == Qt::Horizontal) ? QRectF(QPointF(0, yOffset/2), QSize(xOffset, 0)) : QRectF(QPointF(yOffset/2, 0), QSize(0, xOffset));
	thumbRects.clear();

	DkTimer dt;

	// mouse over effect
	QPoint p = worldMatrix.inverted().map(mapFromGlobal(QCursor::pos()));

	for (int idx = 0; idx < thumbs.size(); idx++) {

		QSharedPointer<DkThumbNailT> thumb = thumbs.at(idx)->getThumb();

		if (thumb->hasImage() == DkThumbNail::exists_not) {
			thumbRects.push_back(QRectF());
			continue;
		}

		QImage img;
		if (thumb->hasImage() == DkThumbNail::loaded)
			img = thumb->getImage();

		QPointF anchor = orientation == Qt::Horizontal ? bufferDim.topRight() : bufferDim.bottomLeft();
		QRectF r = !img.isNull() ? QRectF(anchor, img.size()) : QRectF(anchor, QSize(DkSettings::display.thumbSize, DkSettings::display.thumbSize));
		if (orientation == Qt::Horizontal && height()-yOffset < r.height()*2)
			r.setSize(QSizeF(qFloor(r.width()*(float)(height()-yOffset)/r.height()), height()-yOffset));
		else if (orientation == Qt::Vertical && width()-yOffset < r.width()*2)
			r.setSize(QSizeF(width()-yOffset, qFloor(r.height()*(float)(width()-yOffset)/r.width())));

		// check if the size is still valid
		if (r.width() < 1 || r.height() < 1) 
			continue;	// this brings us in serious problems with the selection

		// center vertically
		if (orientation == Qt::Horizontal)
			r.moveCenter(QPoint(qFloor(r.center().x()), height()/2));
		else
			r.moveCenter(QPoint(width()/2, qFloor(r.center().y())));

		// update the buffer dim
		if (orientation == Qt::Horizontal)
			bufferDim.setRight(qFloor(bufferDim.right() + r.width()) + cvCeil(xOffset/2.0f));
		else
			bufferDim.setBottom(qFloor(bufferDim.bottom() + r.height()) + cvCeil(xOffset/2.0f));
		thumbRects.push_back(r);

		QRectF imgWorldRect = worldMatrix.mapRect(r);

		// update file rect for move to current file timer
		if (scrollToCurrentImage && idx == currentFileIdx)
			newFileRect = imgWorldRect;

		// is the current image within the canvas?
		if (orientation == Qt::Horizontal && imgWorldRect.right() < 0 || orientation == Qt::Vertical && imgWorldRect.bottom() < 0)
			continue;
		if ((orientation == Qt::Horizontal && imgWorldRect.left() > width() || orientation == Qt::Vertical && imgWorldRect.top() > height()) && scrollToCurrentImage) 
			continue;
		else if (orientation == Qt::Horizontal && imgWorldRect.left() > width() || orientation == Qt::Vertical && imgWorldRect.top() > height())
			break;

		if (thumb->hasImage() == DkThumbNail::not_loaded && 
			DkSettings::resources.numThumbsLoading < DkSettings::resources.maxThumbsLoading) {
				thumb->fetchThumb();
				connect(thumb.data(), SIGNAL(thumbLoadedSignal()), this, SLOT(update()));
		}

		bool isLeftGradient = (orientation == Qt::Horizontal && worldMatrix.dx() < 0 && imgWorldRect.left() < leftGradient.finalStop().x()) ||
			(orientation == Qt::Vertical && worldMatrix.dy() < 0 && imgWorldRect.top() < leftGradient.finalStop().y());
		bool isRightGradient = orientation == Qt::Horizontal && imgWorldRect.right() > rightGradient.start().x() ||
			orientation == Qt::Vertical && imgWorldRect.bottom() > rightGradient.start().y();
		// show that there are more images...
		if (isLeftGradient && !img.isNull())
			drawFadeOut(leftGradient, imgWorldRect, &img);
		if (isRightGradient && !img.isNull())
			drawFadeOut(rightGradient, imgWorldRect, &img);

		if (!img.isNull())
			painter->drawImage(r, img, QRect(QPoint(), img.size()));
		else 
			drawNoImgEffect(painter, r);
				
		if (idx == currentFileIdx)
			drawCurrentImgEffect(painter, r);
		else if (idx == selected && r.contains(p))
			drawSelectedEffect(painter, r);


		//painter->fillRect(QRect(0,0,200, 110), leftGradient);
	}
}

void DkFilePreview::drawNoImgEffect(QPainter* painter, const QRectF& r) {

	QBrush oldBrush = painter->brush();
	QPen oldPen = painter->pen();

	QPen noImgPen(DkSettings::display.bgColor);
	painter->setPen(noImgPen);
	painter->setBrush(QColor(0,0,0,0));
	painter->drawRect(r);
	painter->setPen(oldPen);
	painter->setBrush(oldBrush);
}

void DkFilePreview::drawSelectedEffect(QPainter* painter, const QRectF& r) {

	QBrush oldBrush = painter->brush();
	float oldOp = (float)painter->opacity();
	
	// drawing
	painter->setOpacity(0.4);
	painter->setBrush(DkSettings::display.highlightColor);
	painter->drawRect(r);
	
	// reset painter
	painter->setOpacity(oldOp);
	painter->setBrush(oldBrush);
}

void DkFilePreview::drawCurrentImgEffect(QPainter* painter, const QRectF& r) {

	QPen oldPen = painter->pen();
	QBrush oldBrush = painter->brush();
	float oldOp = (float)painter->opacity();

	// draw
	QRectF cr = r;
	cr.setSize(QSize((int)cr.width()+1, (int)cr.height()+1));
	cr.moveCenter(cr.center() + QPointF(-1,-1));

	QPen cPen(DkSettings::display.highlightColor, 1);
	painter->setBrush(QColor(0,0,0,0));
	painter->setOpacity(1.0);
	painter->setPen(cPen);
	painter->drawRect(cr);

	painter->setOpacity(0.5);
	cr.setSize(QSize((int)cr.width()+2, (int)cr.height()+2));
	cr.moveCenter(cr.center() + QPointF(-1,-1));
	painter->drawRect(cr);

	painter->setBrush(oldBrush);
	painter->setOpacity(oldOp);
	painter->setPen(oldPen);
}

void DkFilePreview::drawFadeOut(QLinearGradient gradient, QRectF imgRect, QImage *img) {

	if (img && img->format() == QImage::Format_Indexed8)
		return;

	// compute current scaling
	QPointF scale(img->width()/imgRect.width(), img->height()/imgRect.height());
	QTransform wm;
	wm.scale(scale.x(), scale.y());
	
	if (orientation == Qt::Horizontal)
		wm.translate(-imgRect.left(), 0);
	else
		wm.translate(0, -imgRect.top());

	QLinearGradient imgGradient = gradient;
	
	if (orientation == Qt::Horizontal) {
		imgGradient.setStart(wm.map(gradient.start()).x(), 0);
		imgGradient.setFinalStop(wm.map(gradient.finalStop()).x(), 0);
	}
	else {
		imgGradient.setStart(0, wm.map(gradient.start()).y());
		imgGradient.setFinalStop(0, wm.map(gradient.finalStop()).y());
	}

	QImage mask = *img;
	QPainter painter(&mask);
	painter.fillRect(img->rect(), Qt::black);
	painter.fillRect(img->rect(), imgGradient);
	painter.end();

	img->setAlphaChannel(mask);
}

void DkFilePreview::resizeEvent(QResizeEvent *event) {

	if (event->size() == event->oldSize() && 
		(orientation == Qt::Horizontal && this->width() == parent->width()  ||
		orientation == Qt::Vertical && this->height() == parent->height())) {
	
			qDebug() << "parent size: " << parent->height();
			return;
	}

	if (currentFileIdx >= 0 && isVisible()) {
		scrollToCurrentImage = true;
		moveImageTimer->start();
	}

	// now update...
	borderTrigger = (orientation == Qt::Horizontal) ? (float)width()*winPercent : (float)height()*winPercent;
	int borderTriggerI = qRound(borderTrigger);
	leftGradient.setFinalStop((orientation == Qt::Horizontal) ? QPoint(borderTriggerI, 0) : QPoint(0, borderTriggerI));
	rightGradient.setStart((orientation == Qt::Horizontal) ? QPoint(width()-borderTriggerI, 0) : QPoint(0, height()-borderTriggerI));
	rightGradient.setFinalStop((orientation == Qt::Horizontal) ?  QPoint(width(), 0) : QPoint(0, height()));

	qDebug() << "file preview size: " << event->size();

	//update();
	QWidget::resizeEvent(event);

}

void DkFilePreview::mouseMoveEvent(QMouseEvent *event) {

	if (lastMousePos.isNull()) {
		lastMousePos = event->pos();
		QWidget::mouseMoveEvent(event);
		return;
	}

	if (mouseTrace < 21) {
		mouseTrace += qRound(fabs(QPointF(lastMousePos - event->pos()).manhattanLength()));
		return;
	}

	float eventPos = orientation == Qt::Horizontal ? (float)event->pos().x() : (float)event->pos().y();
	float lastMousePosC = orientation == Qt::Horizontal ? (float)lastMousePos.x() : (float)lastMousePos.y();
	int limit = orientation == Qt::Horizontal ? width() : height();

	if (event->buttons() == Qt::MiddleButton) {

		float enterPosC = orientation == Qt::Horizontal ? (float)enterPos.x() : (float)enterPos.y();
		float dx = std::fabs((float)(enterPosC - eventPos))*0.015f;
		dx = std::exp(dx);

		if (enterPosC - eventPos < 0)
			dx = -dx;

		currentDx = dx;	// update dx
		return;
	}

	int mouseDir = qRound(eventPos - lastMousePosC);

	if (event->buttons() == Qt::LeftButton) {
		currentDx = (float)mouseDir;
		lastMousePos = event->pos();
		selected = -1;
		setCursor(Qt::ClosedHandCursor);
		scrollToCurrentImage = false;
		moveImages();
		return;
	}

	unsetCursor();

	int ndx = limit - qRound(eventPos);
	int pdx = qRound(eventPos);

	bool left = pdx < ndx;
	float dx = (left) ? (float)pdx : (float)ndx;

	if (dx < borderTrigger && (mouseDir < 0 && left || mouseDir > 0 && !left)) {
		dx = std::exp((borderTrigger - dx)/borderTrigger*3);
		currentDx = (left) ? dx : -dx;

		scrollToCurrentImage = false;
		moveImageTimer->start();
	}
	else if (dx > borderTrigger && !scrollToCurrentImage)
		moveImageTimer->stop();

	// select the current thumbnail
	if (dx > borderTrigger*0.5) {

		int oldSelection = selected;
		selected = -1;

		// find out where the mouse is
		for (int idx = 0; idx < thumbRects.size(); idx++) {

			if (worldMatrix.mapRect(thumbRects.at(idx)).contains(event->pos())) {
				selected = idx;

				if (selected <= thumbs.size() && selected >= 0) {
					QSharedPointer<DkThumbNailT> thumb = thumbs.at(selected)->getThumb();
					//selectedImg = DkImage::colorizePixmap(QPixmap::fromImage(thumb->getImage()), DkSettings::display.highlightColor, 0.3f);

					// important: setText shows the label - if you then hide it here again you'll get a stack overflow
					//if (fileLabel->height() < height())
					//	fileLabel->setText(thumbs.at(selected).getFile().fileName(), -1);
					QFileInfo fileInfo(thumb->getFile());
					QString toolTipInfo = tr("Name: ") + thumb->getFile().fileName() + 
						"\n" + tr("Size: ") + DkUtils::readableByte((float)fileInfo.size()) + 
						"\n" + tr("Created: ") + fileInfo.created().toString(Qt::SystemLocaleDate);
					setToolTip(toolTipInfo);
					setStatusTip(thumb->getFile().fileName());
				}
				break;
			}
		}

		if (selected != -1 || selected != oldSelection)
			update();
		//else if (selected == -1)
		//	fileLabel->hide();
	}
	else
		selected = -1;

	if (selected == -1)
		setToolTip(tr("CTRL+Zoom resizes the thumbnails"));


	lastMousePos = event->pos();

	//QWidget::mouseMoveEvent(event);
}

void DkFilePreview::mousePressEvent(QMouseEvent *event) {

	if (event->buttons() == Qt::LeftButton) {
		mouseTrace = 0;
	}
	else if (event->buttons() == Qt::MiddleButton) {

		enterPos = event->pos();
		qDebug() << "stop scrolling (middle button)";
		scrollToCurrentImage = false;
		moveImageTimer->start();

		// show icon
		wheelButton->move(event->pos().x()-16, event->pos().y()-16);
		wheelButton->show();
	}

}

void DkFilePreview::mouseReleaseEvent(QMouseEvent *event) {

	currentDx = 0;
	moveImageTimer->stop();
	wheelButton->hide();
	qDebug() << "stopping image timer (mouse release)";

	if (mouseTrace < 20) {

		// find out where the mouse did click
		for (int idx = 0; idx < thumbRects.size(); idx++) {

			if (idx < thumbs.size() && worldMatrix.mapRect(thumbRects.at(idx)).contains(event->pos())) {
				if (thumbs.at(idx)->isFromZip()) 
					emit changeFileSignal(idx - currentFileIdx);
				else 
					emit loadFileSignal(thumbs.at(idx)->file());
			}
		}
	}
	else
		unsetCursor();

}

void DkFilePreview::wheelEvent(QWheelEvent *event) {

	if (event->modifiers() == Qt::CTRL && windowPosition != pos_dock_hor && windowPosition != pos_dock_ver) {

		int newSize = DkSettings::display.thumbSize;
		newSize += qRound(event->delta()*0.05f);

		// make sure it is even
		if (qRound(newSize*0.5f) != newSize*0.5f)
			newSize++;

		if (newSize < 8)
			newSize = 8;
		else if (newSize > 160)
			newSize = 160;

		if (newSize != DkSettings::display.thumbSize) {
			DkSettings::display.thumbSize = newSize;
			update();
		}
	}
	else {
		
		int fc = (event->delta() > 0) ? -1 : 1;
		
		if (!DkSettings::resources.waitForLastImg) {
			currentFileIdx += fc;
			scrollToCurrentImage = true;
		}
		emit changeFileSignal(fc);
	}
}

void DkFilePreview::leaveEvent(QEvent*) {

	selected = -1;
	if (!scrollToCurrentImage) {
		moveImageTimer->stop();
		qDebug() << "stopping timer (leaveEvent)";
	}
	//fileLabel->hide();
	update();
}

void DkFilePreview::contextMenuEvent(QContextMenuEvent *event) {

	contextMenu->exec(event->globalPos());
	event->accept();

	DkWidget::contextMenuEvent(event);
}

void DkFilePreview::newPosition() {

	QAction* sender = static_cast<QAction*>(QObject::sender());

	if (!sender)
		return;

	int pos = 0;
	Qt::Orientation orient = Qt::Horizontal;

	if (sender == contextMenuActions[cm_pos_west]) {
		pos = pos_west;
		orient = Qt::Vertical;
	}
	else if (sender == contextMenuActions[cm_pos_east]) {
		pos = pos_east;
		orient = Qt::Vertical;
	}
	else if (sender == contextMenuActions[cm_pos_north]) {
		pos = pos_north;
		orient = Qt::Horizontal;
	}
	else if (sender == contextMenuActions[cm_pos_south]) {
		pos = pos_south;
		orient = Qt::Horizontal;
	}
	else if (sender == contextMenuActions[cm_pos_dock_hor]) {
		pos = pos_dock_hor;
		orient = Qt::Horizontal;
	}

	// don't apply twice
	if (windowPosition == pos || pos == pos_dock_hor && windowPosition == pos_dock_ver)
		return;

	windowPosition = pos;
	orientation = orient;
	initOrientations();
	emit positionChangeSignal(windowPosition);

	hide();
	show();

	//emit showThumbsDockSignal(true);
}

void DkFilePreview::moveImages() {

	if (!isVisible()) {
		moveImageTimer->stop();
		return;
	}

	int limit = orientation == Qt::Horizontal ? width() : height();
	float center = orientation == Qt::Horizontal ? (float)newFileRect.center().x() : (float)newFileRect.center().y();

	if (scrollToCurrentImage) {
		float cDist = limit/2.0f - center;

		if (fabs(cDist) < limit) {
			currentDx = sqrt(fabs(cDist))/1.3f;
			if (cDist < 0) currentDx *= -1.0f;
		}
		else
			currentDx = cDist/4.0f;

		if (fabs(currentDx) < 2)
			currentDx = (currentDx < 0) ? -2.0f : 2.0f;

		// end position
		if (fabs(cDist) <= 2) {
			currentDx = limit/2.0f-center;
			moveImageTimer->stop();
			scrollToCurrentImage = false;
		}
		else
			isPainted = false;
	}

	float translation	= orientation == Qt::Horizontal ? (float)worldMatrix.dx() : (float)worldMatrix.dy();
	float bufferPos		= orientation == Qt::Horizontal ? (float)bufferDim.right() : (float)bufferDim.bottom();

	// do not scroll out of the thumbs
	if (translation >= limit*0.5 && currentDx > 0 || translation <= -(bufferPos-limit*0.5+xOffset) && currentDx < 0)
		return;

	// set the last step to match the center of the screen...	(nicer if user scrolls very fast)
	if (translation < limit*0.5 && currentDx > 0 && translation+currentDx > limit*0.5 && currentDx > 0)
		currentDx = limit*0.5f-translation;
	else if (translation > -(bufferPos-limit*0.5+xOffset) && translation+currentDx <= -(bufferPos-limit*0.5+xOffset) && currentDx < 0)
		currentDx = -(bufferPos-limit*0.5f+xOffset+(float)worldMatrix.dx());

	//qDebug() << "currentDx: " << currentDx;
	if (orientation == Qt::Horizontal)
		worldMatrix.translate(currentDx, 0);
	else
		worldMatrix.translate(0, currentDx);
	//qDebug() << "dx: " << worldMatrix.dx();
	update();
}

void DkFilePreview::updateFileIdx(int idx) {

	if (idx == currentFileIdx)
		return;

	currentFileIdx = idx;
	if (currentFileIdx >= 0)
		scrollToCurrentImage = true;
	update();
}

void DkFilePreview::setFileInfo(QSharedPointer<DkImageContainerT> cImage) {

	if (!cImage)
		return;

	int tIdx = -1;

	for (int idx = 0; idx < thumbs.size(); idx++) {
		if (thumbs.at(idx)->file().absoluteFilePath() == cImage->file().absoluteFilePath()) {
			tIdx = idx;
			break;
		}
	}

	//// don't know why we needed this statement
	//// however, if we break here, the file preview
	//// might not update correctly
	//if (tIdx == currentFileIdx) {
	//	return;
	//}

	currentFileIdx = tIdx;
	if (currentFileIdx >= 0)
		scrollToCurrentImage = true;
	update();

}

void DkFilePreview::updateThumbs(QVector<QSharedPointer<DkImageContainerT> > thumbs) {

	this->thumbs = thumbs;

	for (int idx = 0; idx < thumbs.size(); idx++) {
		if (thumbs.at(idx)->isSelected()) {
			currentFileIdx = idx;
			break;
		}
	}

	update();
}

void DkFilePreview::setVisible(bool visible) {

	emit showThumbsDockSignal(visible);

	DkWidget::setVisible(visible);
}

// DkThumbLabel --------------------------------------------------------------------
DkThumbLabel::DkThumbLabel(QSharedPointer<DkThumbNailT> thumb, QGraphicsItem* parent) : QGraphicsObject(parent), text(this) {

	thumbInitialized = false;
	fetchingThumb = false;
	isHovered = false;

	//imgLabel = new QLabel(this);
	//imgLabel->setFocusPolicy(Qt::NoFocus);
	//imgLabel->setAttribute(Qt::WA_TransparentForMouseEvents);
	//imgLabel->setScaledContents(true);
	//imgLabel->setFixedSize(10,10);
	//setStyleSheet("QLabel{background: transparent;}");
	setThumb(thumb);
	setFlag(ItemIsSelectable, true);
	//setFlag(ItemIsMovable, true);	// uncomment this - it's fun : )

	//setFlag(QGraphicsItem::ItemIsSelectable, false);

#if QT_VERSION < 0x050000
	setAcceptsHoverEvents(true);
#else
	setAcceptHoverEvents(true);
#endif
}

DkThumbLabel::~DkThumbLabel() {}

void DkThumbLabel::setThumb(QSharedPointer<DkThumbNailT> thumb) {

	this->thumb = thumb;

	if (thumb.isNull())
		return;

	connect(thumb.data(), SIGNAL(thumbLoadedSignal()), this, SLOT(updateLabel()));
	//setStatusTip(thumb->getFile().fileName());
	QFileInfo fileInfo(thumb->getFile());
	QString toolTipInfo = tr("Name: ") + thumb->getFile().fileName() + 
		"\n" + tr("Size: ") + DkUtils::readableByte((float)fileInfo.size()) + 
		"\n" + tr("Created: ") + fileInfo.created().toString(Qt::SystemLocaleDate);

	setToolTip(toolTipInfo);

	// style dummy
	noImagePen.setColor(QColor(150,150,150));
	noImageBrush = QColor(100,100,100,50);

	QColor col = DkSettings::display.highlightColor;
	col.setAlpha(90);
	selectBrush = col;
	selectPen.setColor(DkSettings::display.highlightColor);
	//selectPen.setWidth(2);
}

QPixmap DkThumbLabel::pixmap() const {

	return icon.pixmap();
}

QRectF DkThumbLabel::boundingRect() const {

	return QRectF(QPoint(0,0), QSize(DkSettings::display.thumbPreviewSize, DkSettings::display.thumbPreviewSize));
}

QPainterPath DkThumbLabel::shape() const {

	QPainterPath path;

	path.addRect(boundingRect());
	return path;
}

void DkThumbLabel::updateLabel() {

	if (thumb.isNull())
		return;

	QPixmap pm;

	if (!thumb->getImage().isNull()) {

		pm = QPixmap::fromImage(thumb->getImage());

		if (DkSettings::display.displaySquaredThumbs) {
			QRect r(QPoint(), pm.size());

			if (r.width() > r.height()) {
				r.setX(qFloor((r.width()-r.height())*0.5f));
				r.setWidth(r.height());
			}
			else {
				r.setY(qFloor((r.height()-r.width())*0.5f));
				r.setHeight(r.width());
			}

			pm = pm.copy(r);
		}
	}
	else
		qDebug() << "update called on empty thumb label!";

	if (!pm.isNull()) {
		icon.setTransformationMode(Qt::SmoothTransformation);
		icon.setPixmap(pm);
		icon.setFlag(ItemIsSelectable, true);
		//QFlags<enum> f;
	}
	if (pm.isNull())
		setFlag(ItemIsSelectable, false);	// if we cannot load it -> disable selection

	// update label
	text.setPos(0, pm.height());
	text.setDefaultTextColor(QColor(255,255,255));
	//text.setTextWidth(icon.boundingRect().width());
	QFont font;
	font.setBold(false);
	font.setPixelSize(10);
	text.setFont(font);
	text.setPlainText(thumb->getFile().fileName());
	text.hide();

	prepareGeometryChange();
	updateSize();
}

void DkThumbLabel::updateSize() {

	if (icon.pixmap().isNull())
		return;

	prepareGeometryChange();

	// resize pixmap label
	int maxSize = qMax(icon.pixmap().width(), icon.pixmap().height());
	int ps = DkSettings::display.thumbPreviewSize;

	if ((float)ps/maxSize != icon.scale()) {
		icon.setScale(1.0f);
		icon.setPos(0,0);

		icon.setScale((float)ps/maxSize);
		icon.moveBy((ps-icon.pixmap().width()*icon.scale())*0.5f, (ps-icon.pixmap().height()*icon.scale())*0.5);
	}

	//update();
}	

void DkThumbLabel::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {

	if (thumb.isNull())
		return;

	if (event->buttons() == Qt::LeftButton && event->modifiers() == Qt::ControlModifier) {
		QString exe = QApplication::applicationFilePath();
		QStringList args;
		args.append(thumb->getFile().absoluteFilePath());

		if (objectName() == "DkNoMacsFrameless")
			args.append("1");	

		QProcess::startDetached(exe, args);
	}
	else {
		QFileInfo file = thumb->getFile();
		qDebug() << "trying to load: " << file.absoluteFilePath();
		emit loadFileSignal(file);
	}
}

void DkThumbLabel::hoverEnterEvent(QGraphicsSceneHoverEvent*) {

	isHovered = true;
	emit showFileSignal(thumb->getFile());
	update();
}

void DkThumbLabel::hoverLeaveEvent(QGraphicsSceneHoverEvent*) {

	isHovered = false;
	emit showFileSignal(QFileInfo());
	update();
}

void DkThumbLabel::setVisible(bool visible) {

	icon.setVisible(visible);
	text.setVisible(visible);
}

void DkThumbLabel::paint(QPainter* painter, const QStyleOptionGraphicsItem* option, QWidget* widget) {
	
	if (!fetchingThumb && thumb->hasImage() == DkThumbNail::not_loaded && 
		DkSettings::resources.numThumbsLoading < DkSettings::resources.maxThumbsLoading*2) {
			thumb->fetchThumb();
			fetchingThumb = true;
	}
	else if (!thumbInitialized && (thumb->hasImage() == DkThumbNail::loaded || thumb->hasImage() == DkThumbNail::exists_not)) {
		updateLabel();
		thumbInitialized = true;
		return;		// exit - otherwise we get paint errors
	}

	//if (!pixmap().isNull()) {
	//	painter->setRenderHint(QPainter::SmoothPixmapTransform);
	//	painter->drawPixmap(boundingRect(), pixmap(), QRectF(QPoint(), pixmap().size()));
	//}
	if (icon.pixmap().isNull() && thumb->hasImage() == DkThumbNail::exists_not) {
		painter->setPen(noImagePen);
		painter->setBrush(noImageBrush);
		painter->drawRect(boundingRect());
	}
	else if (icon.pixmap().isNull()) {
		QColor c = DkSettings::display.highlightColor;
		c.setAlpha(30);
		painter->setPen(noImagePen);
		painter->setBrush(c);

		QRectF r = boundingRect();
		painter->drawRect(r);
	}

	// this is the Qt idea of how to fix the dashed border:
	// http://www.qtcentre.org/threads/23087-How-to-hide-the-dashed-frame-outside-the-QGraphicsItem
	// I don't think it's beautiful...
	QStyleOptionGraphicsItem noSelOption;
	if (option) {
		noSelOption = *option;
		noSelOption.state &= ~QStyle::State_Selected;
	}

	//painter->setRenderHints(QPainter::SmoothPixmapTransform | QPainter::Antialiasing);

	QTransform mt = painter->worldTransform();
	QTransform it = mt;
	it.translate(icon.pos().x(), icon.pos().y());
	it.scale(icon.scale(), icon.scale());

	painter->setTransform(it);
	icon.paint(painter, &noSelOption, widget);
	painter->setTransform(mt);

	// draw text
	if (boundingRect().width() > 50 && DkSettings::display.showThumbLabel) {
		
		QTransform tt = mt;
		tt.translate(0, boundingRect().height()-text.boundingRect().height());

		QRectF r = text.boundingRect();
		r.setWidth(boundingRect().width());
		painter->setPen(Qt::NoPen);
		painter->setWorldTransform(tt);
		painter->setBrush(DkSettings::display.bgColorWidget);
		painter->drawRect(r);
		text.paint(painter, &noSelOption, widget);
		painter->setWorldTransform(mt);
	}

	// render hovered
	if (isHovered) {
		painter->setBrush(QColor(255,255,255,60));
		painter->setPen(noImagePen);
		//painter->setPen(Qt::NoPen);
		painter->drawRect(boundingRect());
	}

	// render selected
	if (isSelected()) {
		painter->setBrush(selectBrush);
		painter->setPen(selectPen);
		painter->drawRect(boundingRect());
	}

}

// DkThumbWidget --------------------------------------------------------------------
DkThumbScene::DkThumbScene(QWidget* parent /* = 0 */) : QGraphicsScene(parent) {

	setObjectName("DkThumbWidget");

	xOffset = 0;
	numCols = 0;
	numRows = 0;
	firstLayout = true;
}

void DkThumbScene::updateLayout() {

	if (thumbLabels.empty())
		return;

	QSize pSize;

	if (!views().empty())
		pSize = QSize(views().first()->viewport()->size());

	xOffset = qCeil(DkSettings::display.thumbPreviewSize*0.1f);
	numCols = qMax(qFloor(((float)pSize.width()-xOffset)/(DkSettings::display.thumbPreviewSize + xOffset)), 1);
	numCols = qMin(thumbLabels.size(), numCols);
	numRows = qCeil((float)thumbLabels.size()/numCols);

	qDebug() << "num rows x num cols: " << numCols*numRows;
	qDebug() << " thumb labels size: " << thumbLabels.size();

	int tso = DkSettings::display.thumbPreviewSize+xOffset;
	setSceneRect(0, 0, numCols*tso+xOffset, numRows*tso+xOffset);
	//int fileIdx = thumbPool->getCurrentFileIdx();

	DkTimer dt;
	int cYOffset = xOffset;

	for (int rIdx = 0; rIdx < numRows; rIdx++) {

		int cXOffset = xOffset;

		for (int cIdx = 0; cIdx < numCols; cIdx++) {

			int tIdx = rIdx*numCols+cIdx;

			if (tIdx < 0 || tIdx >= thumbLabels.size())
				break;

			DkThumbLabel* cLabel = thumbLabels.at(tIdx);
			cLabel->setPos(cXOffset, cYOffset);
			cLabel->updateSize();

			//if (tIdx == fileIdx)
			//	cLabel->ensureVisible();

			//cLabel->show();

			cXOffset += DkSettings::display.thumbPreviewSize + xOffset;
		}

		// update ypos
		cYOffset += DkSettings::display.thumbPreviewSize + xOffset;	// 20 for label 
	}

	qDebug() << "moving takes: " << dt.getTotal();

	for (int idx = 0; idx < thumbLabels.size(); idx++) {

		//if (thumbs.at(idx)->isSelected()) {
		//	thumbLabels.at(idx)->ensureVisible();
		//	thumbLabels.at(idx)->setSelected(true);	// not working here?!
		//}
		if (thumbLabels.at(idx)->isSelected())
			thumbLabels.at(idx)->ensureVisible();
	}

	//update();

	//if (verticalScrollBar()->isVisible())
	//	verticalScrollBar()->update();

	firstLayout = false;
}

void DkThumbScene::updateThumbs(QVector<QSharedPointer<DkImageContainerT> > thumbs) {

	this->thumbs = thumbs;
	updateThumbLabels();
}

void DkThumbScene::updateThumbLabels() {

	qDebug() << "updating thumb labels...";

	DkTimer dt;

	blockSignals(true);	// do not emit selection changed while clearing!
	clear();	// deletes the thumbLabels
	blockSignals(false);

	qDebug() << "clearing viewport: " << dt.getTotal();
	thumbLabels.clear();
	thumbsNotLoaded.clear();

	qDebug() << "clearing labels takes: " << dt.getTotal();

	for (int idx = 0; idx < thumbs.size(); idx++) {
		DkThumbLabel* thumb = new DkThumbLabel(thumbs.at(idx)->getThumb());
		connect(thumb, SIGNAL(loadFileSignal(QFileInfo&)), this, SLOT(loadFile(QFileInfo&)));
		connect(thumb, SIGNAL(showFileSignal(const QFileInfo&)), this, SLOT(showFile(const QFileInfo&)));
		connect(thumbs.at(idx).data(), SIGNAL(thumbLoadedSignal()), this, SIGNAL(thumbLoadedSignal()));

		//thumb->show();
		addItem(thumb);
		thumbLabels.append(thumb);
		//thumbsNotLoaded.append(thumb);
	}

	showFile(QFileInfo());

	qDebug() << "creating labels takes: " << dt.getTotal();

	if (!thumbs.empty())
		updateLayout();

	emit selectionChanged();

	qDebug() << "initializing labels takes: " << dt.getTotal();
}

void DkThumbScene::setImageLoader(QSharedPointer<DkImageLoader> loader) {
	
	connectLoader(this->loader, false);		// disconnect
	this->loader = loader;
	connectLoader(loader);
}

void DkThumbScene::connectLoader(QSharedPointer<DkImageLoader> loader, bool connectSignals) {

	if (!loader)
		return;

	if (connectSignals) {
		connect(loader.data(), SIGNAL(updateDirSignal(QVector<QSharedPointer<DkImageContainerT> >)), this, SLOT(updateThumbs(QVector<QSharedPointer<DkImageContainerT> >)), Qt::UniqueConnection);
	}
	else {
		disconnect(loader.data(), SIGNAL(updateDirSignal(QVector<QSharedPointer<DkImageContainerT> >)), this, SLOT(updateThumbs(QVector<QSharedPointer<DkImageContainerT> >)));
	}
}

void DkThumbScene::showFile(const QFileInfo& file) {

	if (file.absoluteFilePath() == QDir::currentPath() || file.absoluteFilePath().isEmpty())
		emit statusInfoSignal(tr("%1 Images").arg(QString::number(thumbLabels.size())));
	else
		emit statusInfoSignal(file.fileName());
}

void DkThumbScene::ensureVisible(QSharedPointer<DkImageContainerT> img) const {

	if (!img)
		return;

	for (DkThumbLabel* label : thumbLabels) {

		if (label->getThumb()->getFile().absoluteFilePath() == img->file().absoluteFilePath()) {
			label->ensureVisible();
			break;
		}


	}

}

void DkThumbScene::toggleThumbLabels(bool show) {

	DkSettings::display.showThumbLabel = show;

	for (int idx = 0; idx < thumbLabels.size(); idx++)
		thumbLabels.at(idx)->updateLabel();

	//// well, that's not too beautiful
	//if (DkSettings::display.displaySquaredThumbs)
	//	updateLayout();
}

void DkThumbScene::toggleSquaredThumbs(bool squares) {

	DkSettings::display.displaySquaredThumbs = squares;

	for (int idx = 0; idx < thumbLabels.size(); idx++)
		thumbLabels.at(idx)->updateLabel();

	// well, that's not too beautiful
	if (DkSettings::display.displaySquaredThumbs)
		updateLayout();
}

void DkThumbScene::increaseThumbs() {

	resizeThumbs(1.2f);
}

void DkThumbScene::decreaseThumbs() {

	resizeThumbs(0.8f);
}

void DkThumbScene::resizeThumbs(float dx) {

	if (dx < 0)
		dx += 2.0f;

	int newSize = qRound(DkSettings::display.thumbPreviewSize * dx);
	qDebug() << "delta: " << dx;
	qDebug() << "newsize: " << newSize;

	if (newSize > 6 && newSize <= 160) {
		DkSettings::display.thumbPreviewSize = newSize;
		updateLayout();
	}
}

void DkThumbScene::loadFile(QFileInfo& file) {
	emit loadFileSignal(file);
}

void DkThumbScene::selectAllThumbs(bool selected) {

	qDebug() << "selecting...";
	selectThumbs(selected);
}

void DkThumbScene::selectThumbs(bool selected /* = true */, int from /* = 0 */, int to /* = -1 */) {

	if (to == -1)
		to = thumbLabels.size()-1;

	if (from > to) {
		int tmp = to;
		to = from;
		from = tmp;
	}

	blockSignals(true);
	for (int idx = from; idx <= to && idx < thumbLabels.size(); idx++) {
		thumbLabels.at(idx)->setSelected(selected);
	}
	blockSignals(false);
	emit selectionChanged();
}

void DkThumbScene::copySelected() const {

	QStringList fileList = getSelectedFiles();

	if (fileList.empty())
		return;

	QMimeData* mimeData = new QMimeData();

	if (!fileList.empty()) {

		QList<QUrl> urls;
		for (QString cStr : fileList)
			urls.append(QUrl::fromLocalFile(cStr));
		mimeData->setUrls(urls);
		QClipboard* clipboard = QApplication::clipboard();
		clipboard->setMimeData(mimeData);
	}
}

void DkThumbScene::pasteImages() const {

	copyImages(QApplication::clipboard()->mimeData());
}

void DkThumbScene::copyImages(const QMimeData* mimeData) const {

	if (!mimeData || !mimeData->hasUrls() || !loader)
		return;

	QDir dir = loader->getDir();

	for (QUrl url : mimeData->urls()) {

		QFileInfo fileInfo = DkUtils::urlToLocalFile(url);
		QFile file(fileInfo.absoluteFilePath());
		QString newFilePath = QFileInfo(dir, fileInfo.fileName()).absoluteFilePath();

		// ignore existing silently
		if (QFileInfo(newFilePath).exists())
			continue;

		if (!file.copy(newFilePath)) {
			int answer = QMessageBox::critical(qApp->activeWindow(), tr("Error"), tr("Sorry, I cannot copy %1 to %2")
				.arg(fileInfo.absoluteFilePath(), newFilePath), QMessageBox::Ok | QMessageBox::Cancel);

			if (answer == QMessageBox::Cancel) {
				break;
			}
		}
	}

}

void DkThumbScene::deleteSelected() const {

	QStringList fileList = getSelectedFiles();

	if (fileList.empty())
		return;

	int answer = QMessageBox::question(qApp->activeWindow(), tr("Delete Files"), tr("Are you sure you want to permanently delete %1 file(s)?").arg(fileList.size()), QMessageBox::Yes | QMessageBox::No);

	if (answer == QMessageBox::Yes || answer == QMessageBox::Accepted) {
		
		if (loader && fileList.size() > 100)	// saves CPU
			loader->deactivate();

		for (QString fString : fileList) {

			QFileInfo file(fString);
			QFile f(file.absoluteFilePath());

			if (!f.remove()) {
				answer = QMessageBox::critical(qApp->activeWindow(), tr("Error"), tr("Sorry, I cannot delete:\n%1").arg(file.fileName()), QMessageBox::Ok | QMessageBox::Cancel);

				if (answer == QMessageBox::Cancel) {
					break;
				}
			}
		}

		if (loader && fileList.size() > 100)	// saves CPU
			loader->activate();

		if (loader)
			loader->directoryChanged(loader->getDir().absolutePath());
	}
}

void DkThumbScene::renameSelected() const {

	QStringList fileList = getSelectedFiles();

	if (fileList.empty())
		return;

	bool ok;
	QString newFileName = QInputDialog::getText(qApp->activeWindow(), tr("Rename File(s)"),
		tr("New Filename:"), QLineEdit::Normal,
		"", &ok);
	
	if (ok && !newFileName.isEmpty()) {

		for (int idx = 0; idx < fileList.size(); idx++) {

			QFileInfo fileInfo = fileList.at(idx);
			QFile file(fileInfo.absoluteFilePath());
			QString pattern = (fileList.size() == 1) ? newFileName + ".<old>" : newFileName + "<d:3>.<old>";	// no index if just 1 file was added
			DkFileNameConverter converter(fileInfo.fileName(), pattern, idx);
			QFileInfo newFileInfo(fileInfo.dir(), converter.getConvertedFileName());
			if (!file.rename(newFileInfo.absoluteFilePath())) {
				
				int answer = QMessageBox::critical(qApp->activeWindow(), tr("Error"), tr("Sorry, I cannot rename: %1 to %2")
					.arg(fileInfo.fileName(), newFileInfo.fileName()), QMessageBox::Ok | QMessageBox::Cancel);

				if (answer == QMessageBox::Cancel) {
					break;
				}
			}
		}
	}
}

QStringList DkThumbScene::getSelectedFiles() const {

	QStringList fileList;

	for (int idx = 0; idx < thumbLabels.size(); idx++) {

		if (thumbLabels.at(idx) && thumbLabels.at(idx)->isSelected()) {
			fileList.append(thumbLabels.at(idx)->getThumb()->getFile().absoluteFilePath());
		}
	}

	return fileList;
}

int DkThumbScene::findThumb(DkThumbLabel* thumb) const {

	int thumbIdx = -1;

	for (int idx = 0; idx < thumbLabels.size(); idx++) {
		if (thumb == thumbLabels.at(idx)) {
			thumbIdx = idx;
			break;
		}
	}

	return thumbIdx;
}

bool DkThumbScene::allThumbsSelected() const {

	for (DkThumbLabel* label : thumbLabels)
		if (label->flags() & QGraphicsItem::ItemIsSelectable && !label->isSelected())
			return false;

	return true;
}

// DkThumbView --------------------------------------------------------------------
DkThumbsView::DkThumbsView(DkThumbScene* scene, QWidget* parent /* = 0 */) : QGraphicsView(scene, parent) {

	setObjectName("DkThumbsView");
	this->scene = scene;
	connect(scene, SIGNAL(thumbLoadedSignal()), this, SLOT(fetchThumbs()));

	//setDragMode(QGraphicsView::RubberBandDrag);

	setResizeAnchor(QGraphicsView::AnchorUnderMouse);
	setAcceptDrops(true);

	lastShiftIdx = -1;
}

void DkThumbsView::wheelEvent(QWheelEvent *event) {

	if (event->modifiers() == Qt::ControlModifier) {
		scene->resizeThumbs(event->delta()/100.0f);
	}
	else if (event->modifiers() == Qt::NoModifier) {

		if (verticalScrollBar()->isVisible()) {
			verticalScrollBar()->setValue(verticalScrollBar()->value()-event->delta());
			//fetchThumbs();
			//scene->update();
			//scene->invalidate(scene->sceneRect());
		}
	}

	//QWidget::wheelEvent(event);
}

void DkThumbsView::mousePressEvent(QMouseEvent *event) {

	if (event->buttons() == Qt::LeftButton) {
		mousePos = event->pos();
	}

	qDebug() << "mouse pressed";

#if QT_VERSION < 0x050000
	DkThumbLabel* itemClicked = static_cast<DkThumbLabel*>(scene->itemAt(mapToScene(event->pos())));
#else
	DkThumbLabel* itemClicked = static_cast<DkThumbLabel*>(scene->itemAt(mapToScene(event->pos()), QTransform()));
#endif

	// this is a bit of a hack
	// what we want to achieve: if the user is selecting with e.g. shift or ctrl 
	// and he clicks (unintentionally) into the background - the selection would be lost
	// otherwise so we just don't propagate this event
	if (itemClicked || event->modifiers() == Qt::NoModifier)	
		QGraphicsView::mousePressEvent(event);
}

void DkThumbsView::mouseMoveEvent(QMouseEvent *event) {

	if (event->buttons() == Qt::LeftButton) {

		int dist = qRound(QPointF(event->pos()-mousePos).manhattanLength());

		if (dist > QApplication::startDragDistance()) {

			QStringList fileList = scene->getSelectedFiles();

			QMimeData* mimeData = new QMimeData;

			if (!fileList.empty()) {

				QList<QUrl> urls;
				for (QString fStr : fileList)
					urls.append(QUrl::fromLocalFile(fStr));

				mimeData->setUrls(urls);
				QDrag* drag = new QDrag(this);
				drag->setMimeData(mimeData);
				drag->exec(Qt::CopyAction);
			}
		}
	}

	QGraphicsView::mouseMoveEvent(event);
}

void DkThumbsView::mouseReleaseEvent(QMouseEvent *event) {
	
	QGraphicsView::mouseReleaseEvent(event);
	
#if QT_VERSION < 0x050000
	DkThumbLabel* itemClicked = static_cast<DkThumbLabel*>(scene->itemAt(mapToScene(event->pos())));
#else
	DkThumbLabel* itemClicked = static_cast<DkThumbLabel*>(scene->itemAt(mapToScene(event->pos()), QTransform()));
#endif

	if (lastShiftIdx != -1 && event->modifiers() & Qt::ShiftModifier && itemClicked != 0) {
		scene->selectThumbs(true, lastShiftIdx, scene->findThumb(itemClicked));
		qDebug() << "selecting... with SHIFT from: " << lastShiftIdx << " to: " << scene->findThumb(itemClicked);
	}
	else if (itemClicked != 0) {
		lastShiftIdx = scene->findThumb(itemClicked);
		qDebug() << "starting shift: " << lastShiftIdx;
	}
	else
		lastShiftIdx = -1;

}

void DkThumbsView::dragEnterEvent(QDragEnterEvent *event) {

	qDebug() << event->source() << " I am: " << this;

	if (event->source() == this)
		event->acceptProposedAction();
	else if (event->mimeData()->hasUrls()) {
		QUrl url = event->mimeData()->urls().at(0);
		url = url.toLocalFile();

		QFileInfo file = QFileInfo(url.toString());

		// just accept image files
		if (DkUtils::isValid(file))
			event->acceptProposedAction();
		else if (file.isDir())
			event->acceptProposedAction();
	}

	//QGraphicsView::dragEnterEvent(event);
}

void DkThumbsView::dragMoveEvent(QDragMoveEvent *event) {
//
//	qDebug() << event->source() << " I am: " << this;
//
//	if (event->source() == this)
//		event->acceptProposedAction();
//	else if (event->mimeData()->hasUrls()) {
//		QUrl url = event->mimeData()->urls().at(0);
//		url = url.toLocalFile();
//
//		QFileInfo file = QFileInfo(url.toString());
//
//		// just accept image files
//		if (DkImageLoader::isValid(file))
//			event->acceptProposedAction();
//		else if (file.isDir())
//			event->acceptProposedAction();
//	}
//
//	//QGraphicsView::dragEnterEvent(event);
//}

	if (event->source() == this)
		event->acceptProposedAction();
	else if (event->mimeData()->hasUrls()) {
		QUrl url = event->mimeData()->urls().at(0);
		url = url.toLocalFile();
//		QUrl url = event->mimeData()->urls().at(0);
//		url = url.toLocalFile();
//
//		QFileInfo file = QFileInfo(url.toString());
//
//		// just accept image files
//		if (DkImageLoader::isValid(file))
//			event->acceptProposedAction();
//		else if (file.isDir())
//			event->acceptProposedAction();
//	}
//
//	//QGraphicsView::dragMoveEvent(event);
//}

		QFileInfo file = QFileInfo(url.toString());
//
//	if (event->source() == this) {
//		event->accept();
//		return;
//	}
//
//	if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() > 0) {
//		QUrl url = event->mimeData()->urls().at(0);
//		qDebug() << "dropping: " << url;
//		url = url.toLocalFile();
//
//		QFileInfo file = QFileInfo(url.toString());
//		QDir newDir = file.isDir() ? file.absoluteFilePath() : file.absolutePath();
//
//		emit updateDirSignal(newDir);
//	}
//
//	QGraphicsView::dropEvent(event);
//
//	qDebug() << "drop event...";
//}

		// just accept image files
		if (DkUtils::isValid(file))
			event->acceptProposedAction();
		else if (file.isDir())
			event->acceptProposedAction();
	}

	//QGraphicsView::dragMoveEvent(event);
}

void DkThumbsView::dropEvent(QDropEvent *event) {

	if (event->source() == this) {
		event->accept();
		return;
	}

	if (event->mimeData()->hasUrls() && event->mimeData()->urls().size() > 0) {
		
		if (event->mimeData()->urls().size() > 1) {
			scene->copyImages(event->mimeData());
			return;
		}

		QUrl url = event->mimeData()->urls().at(0);
		qDebug() << "dropping: " << url;
		url = url.toLocalFile();

		QFileInfo file = QFileInfo(url.toString());
		QDir newDir = (file.isDir()) ? file.absoluteFilePath() : file.absolutePath();

		emit updateDirSignal(newDir);
	}

	QGraphicsView::dropEvent(event);

	qDebug() << "drop event...";
}

void DkThumbsView::fetchThumbs() {

	int maxThreads = DkSettings::resources.maxThumbsLoading*2;

	// don't do anything if it is loading anyway
	if (DkSettings::resources.numThumbsLoading/* > maxThreads*/) {
		//qDebug() << "rejected because num thumbs loading: " << 
		return;
	}

	qDebug() << "currently loading: " << DkSettings::resources.numThumbsLoading << " thumbs";

	//bool firstReached = false;

	QList<QGraphicsItem*> items = scene->items(mapToScene(viewport()->rect()).boundingRect(), Qt::IntersectsItemShape);

	//qDebug() << mapToScene(viewport()->rect()).boundingRect() << " number of items: " << items.size();

	for (int idx = 0; idx < items.size(); idx++) {

		if (!maxThreads)
			break;

		DkThumbLabel* th = dynamic_cast<DkThumbLabel*>(items.at(idx));

		if (!th) {
			qDebug() << "not a thumb label...";
			continue;
		}

		if (th->pixmap().isNull()) {
			th->update();
			maxThreads--;
		}
		//else if (!thumbLabels.at(idx)->pixmap().isNull())
		//	firstReached = true;
	}

}

// DkThumbScrollWidget --------------------------------------------------------------------
DkThumbScrollWidget::DkThumbScrollWidget(QWidget* parent /* = 0 */, Qt::WindowFlags flags /* = 0 */) : DkWidget(parent, flags) {

	setObjectName("DkThumbScrollWidget");
	setContentsMargins(0,0,0,0);

	thumbsScene = new DkThumbScene(this);
	//thumbsView->setContentsMargins(0,0,0,0);

	view = new DkThumbsView(thumbsScene, this);
	view->setFocusPolicy(Qt::StrongFocus);
	connect(view, SIGNAL(updateDirSignal(QDir)), this, SIGNAL(updateDirSignal(QDir)));
	connect(thumbsScene, SIGNAL(selectionChanged()), this, SLOT(enableSelectionActions()));

	createActions();
	createToolbar();

	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->setContentsMargins(0,0,0,0);
	layout->setSpacing(0);
	layout->addWidget(toolbar);
	layout->addWidget(view);
	setLayout(layout);

	enableSelectionActions();
}

void DkThumbScrollWidget::addContextMenuActions(const QVector<QAction*>& actions, QString menuTitle) {

	parentActions = actions;

	if (!menuTitle.isEmpty()) {
		QMenu* m = contextMenu->addMenu(menuTitle);
		m->addActions(parentActions.toList());

		QToolButton* toolButton = new QToolButton(this);
		toolButton->setObjectName("DkThumbToolButton");
		toolButton->setMenu(m);
		toolButton->setAccessibleName(menuTitle);
		toolButton->setText(menuTitle);

		if (menuTitle == tr("&Sort")) {	// that's an awful hack
			QPixmap pm(":/nomacs/img/sort.png");
			
			if (!DkSettings::display.defaultIconColor || DkSettings::app.privateMode)
				pm = DkImage::colorizePixmap(pm, DkSettings::display.iconColor);
			
			toolButton->setIcon(pm);
		}
		toolButton->setPopupMode(QToolButton::InstantPopup);
		toolbar->insertWidget(actions[action_display_squares], toolButton);

		addActions(actions.toList());
	}
	else {
		contextMenu->addSeparator();
		contextMenu->addActions(parentActions.toList());
	}
}

void DkThumbScrollWidget::createToolbar() {

	toolbar = new QToolBar(tr("Thumb Preview Toolbar"), this);

	if (DkSettings::display.smallIcons)
		toolbar->setIconSize(QSize(16, 16));
	else
		toolbar->setIconSize(QSize(32, 32));

	qDebug() << toolbar->styleSheet();

	if (DkSettings::display.toolbarGradient) {
		toolbar->setObjectName("toolBarWithGradient");
	}

	toolbar->addAction(actions[action_zoom_in]);
	toolbar->addAction(actions[action_zoom_out]);
	toolbar->addAction(actions[action_display_squares]);
	toolbar->addAction(actions[action_show_labels]);
	toolbar->addSeparator();
	toolbar->addAction(actions[action_copy]);
	toolbar->addAction(actions[action_paste]);
	toolbar->addAction(actions[action_rename]);
	toolbar->addAction(actions[action_delete]);
	toolbar->addSeparator();
	toolbar->addAction(actions[action_batch]);

	filterEdit = new QLineEdit("", this);
	filterEdit->setPlaceholderText(tr("Filter Files (Ctrl + F)"));
	filterEdit->setMaximumWidth(250);
	connect(filterEdit, SIGNAL(textChanged(const QString&)), this, SIGNAL(filterChangedSignal(const QString&)));

	// right align search filters
	QWidget* spacer = new QWidget(this);
	spacer->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
	toolbar->addWidget(spacer);
	toolbar->addWidget(filterEdit);
}

void DkThumbScrollWidget::createActions() {

	actions.resize(actions_end);

	actions[action_select_all] = new QAction(tr("Select &All"), this);
	actions[action_select_all]->setShortcut(QKeySequence::SelectAll);
	actions[action_select_all]->setCheckable(true);
	connect(actions[action_select_all], SIGNAL(triggered(bool)), thumbsScene, SLOT(selectAllThumbs(bool)));

	actions[action_zoom_in] = new QAction(QIcon(":/nomacs/img/zoom-in.png"), tr("Zoom &In"), this);
	actions[action_zoom_in]->setShortcut(QKeySequence::ZoomIn);
	connect(actions[action_zoom_in], SIGNAL(triggered()), thumbsScene, SLOT(increaseThumbs()));

	actions[action_zoom_out] = new QAction(QIcon(":/nomacs/img/zoom-out.png"), tr("Zoom &Out"), this);
	actions[action_zoom_out]->setShortcut(QKeySequence::ZoomOut);
	connect(actions[action_zoom_out], SIGNAL(triggered()), thumbsScene, SLOT(decreaseThumbs()));

	actions[action_display_squares] = new QAction(QIcon(":/nomacs/img/thumbs-view.png"), tr("Display &Squares"), this);
	actions[action_display_squares]->setCheckable(true);
	actions[action_display_squares]->setChecked(DkSettings::display.displaySquaredThumbs);
	connect(actions[action_display_squares], SIGNAL(triggered(bool)), thumbsScene, SLOT(toggleSquaredThumbs(bool)));

	actions[action_show_labels] = new QAction(QIcon(":/nomacs/img/show-filename.png"), tr("Show &Filename"), this);
	actions[action_show_labels]->setCheckable(true);
	actions[action_show_labels]->setChecked(DkSettings::display.showThumbLabel);
	connect(actions[action_show_labels], SIGNAL(triggered(bool)), thumbsScene, SLOT(toggleThumbLabels(bool)));

	actions[action_filter] = new QAction(tr("&Filter"), this);
	actions[action_filter]->setShortcut(QKeySequence(Qt::CTRL + Qt::Key_F));
	connect(actions[action_filter], SIGNAL(triggered()), this, SLOT(setFilterFocus()));

	actions[action_delete] = new QAction(QIcon(":/nomacs/img/trash.png"), tr("&Delete"), this);
	actions[action_delete]->setShortcut(QKeySequence::Delete);
	connect(actions[action_delete], SIGNAL(triggered()), thumbsScene, SLOT(deleteSelected()));

	actions[action_copy] = new QAction(QIcon(":/nomacs/img/copy.png"), tr("&Copy"), this);
	actions[action_copy]->setShortcut(QKeySequence::Copy);
	connect(actions[action_copy], SIGNAL(triggered()), thumbsScene, SLOT(copySelected()));

	actions[action_paste] = new QAction(QIcon(":/nomacs/img/paste.png"), tr("&Paste"), this);
	actions[action_paste]->setShortcut(QKeySequence::Paste);
	connect(actions[action_paste], SIGNAL(triggered()), thumbsScene, SLOT(pasteImages()));

	actions[action_rename] = new QAction(QIcon(":/nomacs/img/rename.png"), tr("&Rename"), this);
	actions[action_rename]->setShortcut(QKeySequence(Qt::Key_F2));
	connect(actions[action_rename], SIGNAL(triggered()), thumbsScene, SLOT(renameSelected()));

	actions[action_batch] = new QAction(QIcon(":/nomacs/img/batch-processing.png"), tr("&Batch Process"), this);
	actions[action_batch]->setToolTip(tr("Adds selected files to batch processing."));
	actions[action_batch]->setShortcut(QKeySequence(Qt::Key_B));
	connect(actions[action_batch], SIGNAL(triggered()), this, SLOT(batchProcessFiles()));

	contextMenu = new QMenu(tr("Thumb"), this);
	for (int idx = 0; idx < actions.size(); idx++) {

		actions[idx]->setShortcutContext(Qt::WidgetWithChildrenShortcut);
		contextMenu->addAction(actions.at(idx));

		if (idx == action_show_labels)
			contextMenu->addSeparator();
	}

	// now colorize all icons
	if (!DkSettings::display.defaultIconColor || DkSettings::app.privateMode) {
		for (QAction* action : actions)
			action->setIcon(DkImage::colorizePixmap(action->icon().pixmap(32), DkSettings::display.iconColor));
	}

	addActions(actions.toList());
}

void DkThumbScrollWidget::batchProcessFiles() const {

	QStringList fileList = thumbsScene->getSelectedFiles();
	emit batchProcessFilesSignal(fileList);
}

void DkThumbScrollWidget::updateThumbs(QVector<QSharedPointer<DkImageContainerT> > thumbs) {

	thumbsScene->updateThumbs(thumbs);
}

void DkThumbScrollWidget::clear() {

	thumbsScene->updateThumbs(QVector<QSharedPointer<DkImageContainerT> > ());
}

void DkThumbScrollWidget::setDir(QDir dir) {

	if (isVisible())
		emit updateDirSignal(dir);
}

void DkThumbScrollWidget::setVisible(bool visible) {

	DkWidget::setVisible(visible);

	if (visible) {
		thumbsScene->updateThumbLabels();
		filterEdit->setText("");
		qDebug() << "showing thumb scroll widget...";
	}
}

void DkThumbScrollWidget::setFilterFocus() const {

	filterEdit->setFocus(Qt::MouseFocusReason);
}

void DkThumbScrollWidget::resizeEvent(QResizeEvent *event) {

	if (event->oldSize().width() != event->size().width() && isVisible())
		thumbsScene->updateLayout();

	DkWidget::resizeEvent(event);

}

void DkThumbScrollWidget::contextMenuEvent(QContextMenuEvent *event) {

	//if (!event->isAccepted())
	contextMenu->exec(event->globalPos());
	event->accept();

	//QGraphicsView::contextMenuEvent(event);
}

void DkThumbScrollWidget::enableSelectionActions() {

	bool enable = !thumbsScene->getSelectedFiles().isEmpty();

	actions[action_copy]->setEnabled(enable);
	actions[action_rename]->setEnabled(enable);
	actions[action_delete]->setEnabled(enable);
	actions[action_batch]->setEnabled(enable);

	actions[action_select_all]->setChecked(thumbsScene->allThumbsSelected());
}

}
