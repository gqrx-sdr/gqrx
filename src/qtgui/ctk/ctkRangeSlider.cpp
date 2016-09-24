/*=========================================================================

  Library:   CTK

  Copyright (c) Kitware Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.

=========================================================================*/

// Qt includes
#include <QDebug>
#include <QMouseEvent>
#include <QKeyEvent>
#include <QStyleOptionSlider>
#include <QApplication>
#include <QStylePainter>
#include <QStyle>
#include <QToolTip>

// CTK includes
#include "ctkRangeSlider.h"

class ctkRangeSliderPrivate
{
  Q_DECLARE_PUBLIC(ctkRangeSlider);
protected:
  ctkRangeSlider* const q_ptr;
public:
  /// Boolean indicates the selected handle
  ///   True for the minimum range handle, false for the maximum range handle
  enum Handle {
    NoHandle = 0x0,
    MinimumHandle = 0x1,
    MaximumHandle = 0x2
  };
  Q_DECLARE_FLAGS(Handles, Handle);

  ctkRangeSliderPrivate(ctkRangeSlider& object);
  void init();

  /// Return the handle at the given pos, or none if no handle is at the pos.
  /// If a handle is selected, handleRect is set to the handle rect.
  /// otherwise return NoHandle and handleRect is set to the combined rect of
  /// the min and max handles
  Handle handleAtPos(const QPoint& pos, QRect& handleRect)const;

  /// Copied verbatim from QSliderPrivate class (see QSlider.cpp)
  int pixelPosToRangeValue(int pos) const;
  int pixelPosFromRangeValue(int val) const;

  /// Draw the bottom and top sliders.
  void drawMinimumSlider( QStylePainter* painter ) const;
  void drawMaximumSlider( QStylePainter* painter ) const;
    
  /// End points of the range on the Model
  int m_MaximumValue;
  int m_MinimumValue;

  /// End points of the range on the GUI. This is synced with the model.
  int m_MaximumPosition;
  int m_MinimumPosition;

  /// Controls selected ?
  QStyle::SubControl m_MinimumSliderSelected;
  QStyle::SubControl m_MaximumSliderSelected;

  /// See QSliderPrivate::clickOffset. 
  /// Overrides this ivar
  int m_SubclassClickOffset;
    
  /// See QSliderPrivate::position
  /// Overrides this ivar.
  int m_SubclassPosition;
  
  /// Original width between the 2 bounds before any moves
  float m_SubclassWidth;

  ctkRangeSliderPrivate::Handles m_SelectedHandles;

  /// When symmetricMoves is true, moving a handle will move the other handle
  /// symmetrically, otherwise the handles are independent.
  bool m_SymmetricMoves;

  QString m_HandleToolTip;

private:
  Q_DISABLE_COPY(ctkRangeSliderPrivate);
};

// --------------------------------------------------------------------------
ctkRangeSliderPrivate::ctkRangeSliderPrivate(ctkRangeSlider& object)
  :q_ptr(&object)
{
  this->m_MinimumValue = 0;
  this->m_MaximumValue = 100;
  this->m_MinimumPosition = 0;
  this->m_MaximumPosition = 100;
  this->m_MinimumSliderSelected = QStyle::SC_None;
  this->m_MaximumSliderSelected = QStyle::SC_None;
  this->m_SubclassClickOffset = 0;
  this->m_SubclassPosition = 0;
  this->m_SubclassWidth = 0.0;
  this->m_SelectedHandles = 0;
  this->m_SymmetricMoves = false;
}

// --------------------------------------------------------------------------
void ctkRangeSliderPrivate::init()
{
  Q_Q(ctkRangeSlider);
  this->m_MinimumValue = q->minimum();
  this->m_MaximumValue = q->maximum();
  this->m_MinimumPosition = q->minimum();
  this->m_MaximumPosition = q->maximum();
  q->connect(q, SIGNAL(rangeChanged(int,int)), q, SLOT(onRangeChanged(int,int)));
}

// --------------------------------------------------------------------------
ctkRangeSliderPrivate::Handle ctkRangeSliderPrivate::handleAtPos(const QPoint& pos, QRect& handleRect)const
{
  Q_Q(const ctkRangeSlider);

  QStyleOptionSlider option;
  q->initStyleOption( &option );

  // The functinos hitTestComplexControl only know about 1 handle. As we have
  // 2, we change the position of the handle and test if the pos correspond to
  // any of the 2 positions.
  
  // Test the MinimumHandle
  option.sliderPosition = this->m_MinimumPosition;
  option.sliderValue    = this->m_MinimumValue;

  QStyle::SubControl minimumControl = q->style()->hitTestComplexControl(
    QStyle::CC_Slider, &option, pos, q);
  QRect minimumHandleRect = q->style()->subControlRect(
      QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, q);

  // Test if the pos is under the Maximum handle 
  option.sliderPosition = this->m_MaximumPosition;
  option.sliderValue    = this->m_MaximumValue;

  QStyle::SubControl maximumControl = q->style()->hitTestComplexControl(
    QStyle::CC_Slider, &option, pos, q);
  QRect maximumHandleRect = q->style()->subControlRect(
      QStyle::CC_Slider, &option, QStyle::SC_SliderHandle, q);

  // The pos is above both handles, select the closest handle
  if (minimumControl == QStyle::SC_SliderHandle &&
      maximumControl == QStyle::SC_SliderHandle)
    {
    int minDist = 0;
    int maxDist = 0;
    if (q->orientation() == Qt::Horizontal)
      {
      minDist = pos.x() - minimumHandleRect.left();
      maxDist = maximumHandleRect.right() - pos.x(); 
      }
    else //if (q->orientation() == Qt::Vertical)
      {
      minDist = minimumHandleRect.bottom() - pos.y();
      maxDist = pos.y() - maximumHandleRect.top(); 
      }
    Q_ASSERT( minDist >= 0 && maxDist >= 0);
    minimumControl = minDist < maxDist ? minimumControl : QStyle::SC_None;
    }

  if (minimumControl == QStyle::SC_SliderHandle)
    {
    handleRect = minimumHandleRect;
    return MinimumHandle;
    }
  else if (maximumControl == QStyle::SC_SliderHandle)
    {
    handleRect = maximumHandleRect;
    return MaximumHandle;
    }
  handleRect = minimumHandleRect.united(maximumHandleRect);
  return NoHandle;
}

// --------------------------------------------------------------------------
// Copied verbatim from QSliderPrivate::pixelPosToRangeValue. See QSlider.cpp
//
int ctkRangeSliderPrivate::pixelPosToRangeValue( int pos ) const
{
  Q_Q(const ctkRangeSlider);
  QStyleOptionSlider option;
  q->initStyleOption( &option );

  QRect gr = q->style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderGroove, 
                                            q );
  QRect sr = q->style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderHandle, 
                                            q );
  int sliderMin, sliderMax, sliderLength;
  if (option.orientation == Qt::Horizontal) 
    {
    sliderLength = sr.width();
    sliderMin = gr.x();
    sliderMax = gr.right() - sliderLength + 1;
    }
  else
    {
    sliderLength = sr.height();
    sliderMin = gr.y();
    sliderMax = gr.bottom() - sliderLength + 1;
    }

  return QStyle::sliderValueFromPosition( q->minimum(), 
                                          q->maximum(), 
                                          pos - sliderMin,
                                          sliderMax - sliderMin, 
                                          option.upsideDown );
}

//---------------------------------------------------------------------------
int ctkRangeSliderPrivate::pixelPosFromRangeValue( int val ) const
{
  Q_Q(const ctkRangeSlider);
  QStyleOptionSlider option;
  q->initStyleOption( &option );

  QRect gr = q->style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderGroove, 
                                            q );
  QRect sr = q->style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderHandle, 
                                            q );
  int sliderMin, sliderMax, sliderLength;
  if (option.orientation == Qt::Horizontal) 
    {
    sliderLength = sr.width();
    sliderMin = gr.x();
    sliderMax = gr.right() - sliderLength + 1;
    }
  else
    {
    sliderLength = sr.height();
    sliderMin = gr.y();
    sliderMax = gr.bottom() - sliderLength + 1;
    }

  return QStyle::sliderPositionFromValue( q->minimum(), 
                                          q->maximum(), 
                                          val,
                                          sliderMax - sliderMin, 
                                          option.upsideDown ) + sliderMin;
}

//---------------------------------------------------------------------------
// Draw slider at the bottom end of the range
void ctkRangeSliderPrivate::drawMinimumSlider( QStylePainter* painter ) const
{
  Q_Q(const ctkRangeSlider);
  QStyleOptionSlider option;
  q->initMinimumSliderStyleOption( &option );

  option.subControls = QStyle::SC_SliderHandle;
  option.sliderValue = m_MinimumValue;
  option.sliderPosition = m_MinimumPosition;
  if (q->isMinimumSliderDown())
    {
    option.activeSubControls = QStyle::SC_SliderHandle;
    option.state |= QStyle::State_Sunken;
    }
#ifdef Q_OS_MAC
  // On mac style, drawing just the handle actually draws also the groove.
  QRect clip = q->style()->subControlRect(QStyle::CC_Slider, &option,
                                          QStyle::SC_SliderHandle, q);
  painter->setClipRect(clip);
#endif
  painter->drawComplexControl(QStyle::CC_Slider, option);
}

//---------------------------------------------------------------------------
// Draw slider at the top end of the range
void ctkRangeSliderPrivate::drawMaximumSlider( QStylePainter* painter ) const
{
  Q_Q(const ctkRangeSlider);
  QStyleOptionSlider option;
  q->initMaximumSliderStyleOption( &option );

  option.subControls = QStyle::SC_SliderHandle;
  option.sliderValue = m_MaximumValue;
  option.sliderPosition = m_MaximumPosition;
  if (q->isMaximumSliderDown())
    {
    option.activeSubControls = QStyle::SC_SliderHandle;
    option.state |= QStyle::State_Sunken;
    }
#ifdef Q_OS_MAC
  // On mac style, drawing just the handle actually draws also the groove.
  QRect clip = q->style()->subControlRect(QStyle::CC_Slider, &option,
                                          QStyle::SC_SliderHandle, q);
  painter->setClipRect(clip);
#endif
  painter->drawComplexControl(QStyle::CC_Slider, option);
}

// --------------------------------------------------------------------------
ctkRangeSlider::ctkRangeSlider(QWidget* _parent)
  : QSlider(_parent)
  , d_ptr(new ctkRangeSliderPrivate(*this))
{
  Q_D(ctkRangeSlider);
  d->init();
}

// --------------------------------------------------------------------------
ctkRangeSlider::ctkRangeSlider( Qt::Orientation o,
                                  QWidget* parentObject )
  :QSlider(o, parentObject)
  , d_ptr(new ctkRangeSliderPrivate(*this))
{
  Q_D(ctkRangeSlider);
  d->init();
}

// --------------------------------------------------------------------------
ctkRangeSlider::ctkRangeSlider(ctkRangeSliderPrivate* impl, QWidget* _parent)
  : QSlider(_parent)
  , d_ptr(impl)
{
  Q_D(ctkRangeSlider);
  d->init();
}

// --------------------------------------------------------------------------
ctkRangeSlider::ctkRangeSlider( ctkRangeSliderPrivate* impl, Qt::Orientation o,
                                QWidget* parentObject )
  :QSlider(o, parentObject)
  , d_ptr(impl)
{
  Q_D(ctkRangeSlider);
  d->init();
}

// --------------------------------------------------------------------------
ctkRangeSlider::~ctkRangeSlider()
{
}

// --------------------------------------------------------------------------
int ctkRangeSlider::minimumValue() const
{
  Q_D(const ctkRangeSlider);
  return d->m_MinimumValue;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setMinimumValue( int min )
{
  Q_D(ctkRangeSlider);
  this->setValues( min, qMax(d->m_MaximumValue,min) );
}

// --------------------------------------------------------------------------
int ctkRangeSlider::maximumValue() const
{
  Q_D(const ctkRangeSlider);
  return d->m_MaximumValue;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setMaximumValue( int max )
{
  Q_D(ctkRangeSlider);
  this->setValues( qMin(d->m_MinimumValue, max), max );
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setValues(int l, int u)
{
  Q_D(ctkRangeSlider);
  const int minValue = 
    qBound(this->minimum(), qMin(l,u), this->maximum());
  const int maxValue = 
    qBound(this->minimum(), qMax(l,u), this->maximum());
  bool emitMinValChanged = (minValue != d->m_MinimumValue);
  bool emitMaxValChanged = (maxValue != d->m_MaximumValue);
  
  d->m_MinimumValue = minValue;
  d->m_MaximumValue = maxValue;
  
  bool emitMinPosChanged = 
    (minValue != d->m_MinimumPosition);
  bool emitMaxPosChanged = 
    (maxValue != d->m_MaximumPosition);
  d->m_MinimumPosition = minValue;
  d->m_MaximumPosition = maxValue;
  
  if (isSliderDown())
    {
    if (emitMinPosChanged || emitMaxPosChanged)
      {
      emit positionsChanged(d->m_MinimumPosition, d->m_MaximumPosition);
      }
    if (emitMinPosChanged)
      {
      emit minimumPositionChanged(d->m_MinimumPosition);
      }
    if (emitMaxPosChanged)
      {
      emit maximumPositionChanged(d->m_MaximumPosition);
      }
    }
  if (emitMinValChanged || emitMaxValChanged)
    {
    emit valuesChanged(d->m_MinimumValue, 
                       d->m_MaximumValue);
    }
  if (emitMinValChanged)
    {
    emit minimumValueChanged(d->m_MinimumValue);
    }
  if (emitMaxValChanged)
    {
    emit maximumValueChanged(d->m_MaximumValue);
    }
  if (emitMinPosChanged || emitMaxPosChanged || 
      emitMinValChanged || emitMaxValChanged)
    {
    this->update();
    }
}

// --------------------------------------------------------------------------
int ctkRangeSlider::minimumPosition() const
{
  Q_D(const ctkRangeSlider);
  return d->m_MinimumPosition;
}

// --------------------------------------------------------------------------
int ctkRangeSlider::maximumPosition() const
{
  Q_D(const ctkRangeSlider);
  return d->m_MaximumPosition;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setMinimumPosition(int l)
{
  Q_D(const ctkRangeSlider);
  this->setPositions(l, qMax(l, d->m_MaximumPosition));
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setMaximumPosition(int u)
{
  Q_D(const ctkRangeSlider);
  this->setPositions(qMin(d->m_MinimumPosition, u), u);
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setPositions(int min, int max)
{
  Q_D(ctkRangeSlider);
  const int minPosition = 
    qBound(this->minimum(), qMin(min, max), this->maximum());
  const int maxPosition = 
    qBound(this->minimum(), qMax(min, max), this->maximum());

  bool emitMinPosChanged = (minPosition != d->m_MinimumPosition);
  bool emitMaxPosChanged = (maxPosition != d->m_MaximumPosition);
  
  if (!emitMinPosChanged && !emitMaxPosChanged)
    {
    return;
    }

  d->m_MinimumPosition = minPosition;
  d->m_MaximumPosition = maxPosition;

  if (!this->hasTracking())
    {
    this->update();
    }
  if (isSliderDown())
    {
    if (emitMinPosChanged)
      {
      emit minimumPositionChanged(d->m_MinimumPosition);
      }
    if (emitMaxPosChanged)
      {
      emit maximumPositionChanged(d->m_MaximumPosition);
      }
    if (emitMinPosChanged || emitMaxPosChanged)
      {
      emit positionsChanged(d->m_MinimumPosition, d->m_MaximumPosition);
      }
    }
  if (this->hasTracking())
    {
    this->triggerAction(SliderMove);
    this->setValues(d->m_MinimumPosition, d->m_MaximumPosition);
    }
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setSymmetricMoves(bool symmetry)
{
  Q_D(ctkRangeSlider);
  d->m_SymmetricMoves = symmetry;
}

// --------------------------------------------------------------------------
bool ctkRangeSlider::symmetricMoves()const
{
  Q_D(const ctkRangeSlider);
  return d->m_SymmetricMoves;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::onRangeChanged(int _minimum, int _maximum)
{
  Q_UNUSED(_minimum);
  Q_UNUSED(_maximum);
  Q_D(ctkRangeSlider);
  this->setValues(d->m_MinimumValue, d->m_MaximumValue);
}

// --------------------------------------------------------------------------
// Render
void ctkRangeSlider::paintEvent( QPaintEvent* )
{
  Q_D(ctkRangeSlider);
  QStyleOptionSlider option;
  this->initStyleOption(&option);

  QStylePainter painter(this);
  option.subControls = QStyle::SC_SliderGroove;
  // Move to minimum to not highlight the SliderGroove.
  // On mac style, drawing just the slider groove also draws the handles,
  // therefore we give a negative (outside of view) position.
  option.sliderValue = this->minimum() - this->maximum();
  option.sliderPosition = this->minimum() - this->maximum();
  painter.drawComplexControl(QStyle::CC_Slider, option);

  option.sliderPosition = d->m_MinimumPosition;
  const QRect lr = style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderHandle, 
                                            this);
  option.sliderPosition = d->m_MaximumPosition;

  const QRect ur = style()->subControlRect( QStyle::CC_Slider, 
                                            &option, 
                                            QStyle::SC_SliderHandle, 
                                            this);

  QRect sr = style()->subControlRect( QStyle::CC_Slider, 
                                      &option, 
                                      QStyle::SC_SliderGroove, 
                                      this);
  QRect rangeBox;
  if (option.orientation == Qt::Horizontal)
    {
    rangeBox = QRect(
      QPoint(qMin( lr.center().x(), ur.center().x() ), sr.center().y() - 2),
      QPoint(qMax( lr.center().x(), ur.center().x() ), sr.center().y() + 1));
    }
  else
    {
    rangeBox = QRect(
      QPoint(sr.center().x() - 2, qMin( lr.center().y(), ur.center().y() )),
      QPoint(sr.center().x() + 1, qMax( lr.center().y(), ur.center().y() )));
    }

  // -----------------------------
  // Render the range
  //
  QRect groove = this->style()->subControlRect( QStyle::CC_Slider, 
                                                &option, 
                                                QStyle::SC_SliderGroove, 
                                                this );
  groove.adjust(0, 0, -1, 0);

  // Create default colors based on the transfer function.
  //
  QColor highlight = this->palette().color(QPalette::Normal, QPalette::Highlight);
  QLinearGradient gradient;
  if (option.orientation == Qt::Horizontal)
    {
    gradient = QLinearGradient( groove.center().x(), groove.top(),
                                groove.center().x(), groove.bottom());
    }
  else
    {
    gradient = QLinearGradient( groove.left(), groove.center().y(),
                                groove.right(), groove.center().y());
    }

  // TODO: Set this based on the supplied transfer function
  //QColor l = Qt::darkGray;
  //QColor u = Qt::black;

  gradient.setColorAt(0, highlight.darker(120));
  gradient.setColorAt(1, highlight.lighter(160));

  painter.setPen(QPen(highlight.darker(150), 0));
  painter.setBrush(gradient);
  painter.drawRect( rangeBox.intersected(groove) );

  //  -----------------------------------
  // Render the sliders
  //
  if (this->isMinimumSliderDown())
    {
    d->drawMaximumSlider( &painter );
    d->drawMinimumSlider( &painter );
    }
  else
    {
    d->drawMinimumSlider( &painter );
    d->drawMaximumSlider( &painter );
    }
}

// --------------------------------------------------------------------------
// Standard Qt UI events
void ctkRangeSlider::mousePressEvent(QMouseEvent* mouseEvent)
{
  Q_D(ctkRangeSlider);
  if (minimum() == maximum() || (mouseEvent->buttons() ^ mouseEvent->button()))
    {
    mouseEvent->ignore();
    return;
    }
  int mepos = this->orientation() == Qt::Horizontal ?
    mouseEvent->pos().x() : mouseEvent->pos().y();

  QStyleOptionSlider option;
  this->initStyleOption( &option );

  QRect handleRect;
  ctkRangeSliderPrivate::Handle handle_ = d->handleAtPos(mouseEvent->pos(), handleRect);

  if (handle_ != ctkRangeSliderPrivate::NoHandle)
    {
    d->m_SubclassPosition = (handle_ == ctkRangeSliderPrivate::MinimumHandle)?
      d->m_MinimumPosition : d->m_MaximumPosition;

    // save the position of the mouse inside the handle for later
    d->m_SubclassClickOffset = mepos - (this->orientation() == Qt::Horizontal ?
      handleRect.left() : handleRect.top());

    this->setSliderDown(true);

    if (d->m_SelectedHandles != handle_)
      {
      d->m_SelectedHandles = handle_;
      this->update(handleRect);
      }
    // Accept the mouseEvent
    mouseEvent->accept();
    return;
    }

  // if we are here, no handles have been pressed
  // Check if we pressed on the groove between the 2 handles
  
  QStyle::SubControl control = this->style()->hitTestComplexControl(
    QStyle::CC_Slider, &option, mouseEvent->pos(), this);
  QRect sr = style()->subControlRect(
    QStyle::CC_Slider, &option, QStyle::SC_SliderGroove, this);
  int minCenter = (this->orientation() == Qt::Horizontal ?
    handleRect.left() : handleRect.top());
  int maxCenter = (this->orientation() == Qt::Horizontal ?
    handleRect.right() : handleRect.bottom());
  if (control == QStyle::SC_SliderGroove &&
      mepos > minCenter && mepos < maxCenter)
    {
    // warning lost of precision it might be fatal
    d->m_SubclassPosition = (d->m_MinimumPosition + d->m_MaximumPosition) / 2.;
    d->m_SubclassClickOffset = mepos - d->pixelPosFromRangeValue(d->m_SubclassPosition);
    d->m_SubclassWidth = (d->m_MaximumPosition - d->m_MinimumPosition) / 2.;
    qMax(d->m_SubclassPosition - d->m_MinimumPosition, d->m_MaximumPosition - d->m_SubclassPosition);
    this->setSliderDown(true);
    if (!this->isMinimumSliderDown() || !this->isMaximumSliderDown())
      {
      d->m_SelectedHandles = 
        QFlags<ctkRangeSliderPrivate::Handle>(ctkRangeSliderPrivate::MinimumHandle) | 
        QFlags<ctkRangeSliderPrivate::Handle>(ctkRangeSliderPrivate::MaximumHandle);
      this->update(handleRect.united(sr));
      }
    mouseEvent->accept();
    return;
    }
  mouseEvent->ignore();
}

// --------------------------------------------------------------------------
// Standard Qt UI events
void ctkRangeSlider::mouseMoveEvent(QMouseEvent* mouseEvent)
{
  Q_D(ctkRangeSlider);
  if (!d->m_SelectedHandles)
    {
    mouseEvent->ignore();
    return;
    }
  int mepos = this->orientation() == Qt::Horizontal ?
    mouseEvent->pos().x() : mouseEvent->pos().y();

  QStyleOptionSlider option;
  this->initStyleOption(&option);

  const int m = style()->pixelMetric( QStyle::PM_MaximumDragDistance, &option, this );

  int newPosition = d->pixelPosToRangeValue(mepos - d->m_SubclassClickOffset);

  if (m >= 0)
    {
    const QRect r = rect().adjusted(-m, -m, m, m);
    if (!r.contains(mouseEvent->pos()))
      {
      newPosition = d->m_SubclassPosition;
      }
    }

  // Only the lower/left slider is down
  if (this->isMinimumSliderDown() && !this->isMaximumSliderDown())
    {
    double newMinPos = qMin(newPosition,d->m_MaximumPosition);
    this->setPositions(newMinPos, d->m_MaximumPosition +
      (d->m_SymmetricMoves ? d->m_MinimumPosition - newMinPos : 0));
    }
  // Only the upper/right slider is down
  else if (this->isMaximumSliderDown() && !this->isMinimumSliderDown())
    {
    double newMaxPos = qMax(d->m_MinimumPosition, newPosition);
    this->setPositions(d->m_MinimumPosition -
      (d->m_SymmetricMoves ? newMaxPos - d->m_MaximumPosition: 0),
      newMaxPos);
    }
  // Both handles are down (the user clicked in between the handles)
  else if (this->isMinimumSliderDown() && this->isMaximumSliderDown())
    {
    this->setPositions(newPosition - static_cast<int>(d->m_SubclassWidth),
                       newPosition + static_cast<int>(d->m_SubclassWidth + .5));
    }
  mouseEvent->accept();
}

// --------------------------------------------------------------------------
// Standard Qt UI mouseEvents
void ctkRangeSlider::mouseReleaseEvent(QMouseEvent* mouseEvent)
{
  Q_D(ctkRangeSlider);
  this->QSlider::mouseReleaseEvent(mouseEvent);

  setSliderDown(false);
  d->m_SelectedHandles = 0;

  this->update();
}

// --------------------------------------------------------------------------
bool ctkRangeSlider::isMinimumSliderDown()const
{
  Q_D(const ctkRangeSlider);
  return d->m_SelectedHandles & ctkRangeSliderPrivate::MinimumHandle;
}

// --------------------------------------------------------------------------
bool ctkRangeSlider::isMaximumSliderDown()const
{
  Q_D(const ctkRangeSlider);
  return d->m_SelectedHandles & ctkRangeSliderPrivate::MaximumHandle;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::initMinimumSliderStyleOption(QStyleOptionSlider* option) const
{
  this->initStyleOption(option);
}

// --------------------------------------------------------------------------
void ctkRangeSlider::initMaximumSliderStyleOption(QStyleOptionSlider* option) const
{
  this->initStyleOption(option);
}

// --------------------------------------------------------------------------
QString ctkRangeSlider::handleToolTip()const
{
  Q_D(const ctkRangeSlider);
  return d->m_HandleToolTip;
}

// --------------------------------------------------------------------------
void ctkRangeSlider::setHandleToolTip(const QString& _toolTip)
{
  Q_D(ctkRangeSlider);
  d->m_HandleToolTip = _toolTip;
}

// --------------------------------------------------------------------------
bool ctkRangeSlider::event(QEvent* _event)
{
  Q_D(ctkRangeSlider);
  switch(_event->type())
    {
    case QEvent::ToolTip:
      {
      QHelpEvent* helpEvent = static_cast<QHelpEvent*>(_event);
      QStyleOptionSlider opt;
      // Test the MinimumHandle
      opt.sliderPosition = d->m_MinimumPosition;
      opt.sliderValue = d->m_MinimumValue;
      this->initStyleOption(&opt);
      QStyle::SubControl hoveredControl =
        this->style()->hitTestComplexControl(
          QStyle::CC_Slider, &opt, helpEvent->pos(), this);
      if (!d->m_HandleToolTip.isEmpty() &&
          hoveredControl == QStyle::SC_SliderHandle)
        {
        QToolTip::showText(helpEvent->globalPos(), d->m_HandleToolTip.arg(this->minimumValue()));
        _event->accept();
        return true;
        }
      // Test the MaximumHandle
      opt.sliderPosition = d->m_MaximumPosition;
      opt.sliderValue = d->m_MaximumValue;
      this->initStyleOption(&opt);
      hoveredControl = this->style()->hitTestComplexControl(
        QStyle::CC_Slider, &opt, helpEvent->pos(), this);
      if (!d->m_HandleToolTip.isEmpty() &&
          hoveredControl == QStyle::SC_SliderHandle)
        {
        QToolTip::showText(helpEvent->globalPos(), d->m_HandleToolTip.arg(this->maximumValue()));
        _event->accept();
        return true;
        }
      }
    default:
      break;
    }
  return this->Superclass::event(_event);
}
