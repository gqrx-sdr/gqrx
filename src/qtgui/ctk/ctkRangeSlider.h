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

#ifndef __ctkRangeSlider_h
#define __ctkRangeSlider_h

// Qt includes
#include <QSlider>

// CTK includes
#include "ctkPimpl.h"

class QStylePainter;
class ctkRangeSliderPrivate;

/// \ingroup Widgets
///
/// A ctkRangeSlider is a slider that lets you input 2 values instead of one
/// (see QSlider). These values are typically a lower and upper bound.
/// Values are comprised between the range of the slider. See setRange(), 
/// minimum() and maximum(). The upper bound can't be smaller than the 
/// lower bound and vice-versa.
/// When setting new values (setMinimumValue(), setMaximumValue() or
/// setValues()), make sure they lie between the range (minimum(), maximum())
/// of the slider, they would be forced otherwised. If it is not the behavior
/// you desire, you can set the range first (setRange(), setMinimum(),
/// setMaximum())
/// TODO: support triggerAction(QAbstractSlider::SliderSingleStepSub) that
/// moves both values at a time.
/// \sa ctkDoubleRangeSlider, ctkDoubleSlider, ctkRangeWidget
class ctkRangeSlider : public QSlider
{
  Q_OBJECT
  Q_PROPERTY(int minimumValue READ minimumValue WRITE setMinimumValue)
  Q_PROPERTY(int maximumValue READ maximumValue WRITE setMaximumValue)
  Q_PROPERTY(int minimumPosition READ minimumPosition WRITE setMinimumPosition)
  Q_PROPERTY(int maximumPosition READ maximumPosition WRITE setMaximumPosition)
  Q_PROPERTY(bool symmetricMoves READ symmetricMoves WRITE setSymmetricMoves)
  Q_PROPERTY(QString handleToolTip READ handleToolTip WRITE setHandleToolTip)

public:
  // Superclass typedef
  typedef QSlider Superclass;
  /// Constructor, builds a ctkRangeSlider that ranges from 0 to 100 and has
  /// a lower and upper values of 0 and 100 respectively, other properties
  /// are set the QSlider default properties.
  explicit ctkRangeSlider( Qt::Orientation o, QWidget* par= 0 );
  explicit ctkRangeSlider( QWidget* par = 0 );
  virtual ~ctkRangeSlider();

  /// 
  /// This property holds the slider's current minimum value.
  /// The slider silently forces minimumValue to be within the legal range: 
  /// minimum() <= minimumValue() <= maximumValue() <= maximum().
  /// Changing the minimumValue also changes the minimumPosition.
  int minimumValue() const;

  /// 
  /// This property holds the slider's current maximum value.
  /// The slider forces the maximum value to be within the legal range: 
  /// The slider silently forces maximumValue to be within the legal range: 
  /// Changing the maximumValue also changes the maximumPosition.
  int maximumValue() const;

  /// 
  /// This property holds the current slider minimum position.
  /// If tracking is enabled (the default), this is identical to minimumValue.
  int minimumPosition() const;
  void setMinimumPosition(int min);

  /// 
  /// This property holds the current slider maximum position.
  /// If tracking is enabled (the default), this is identical to maximumValue.
  int maximumPosition() const;
  void setMaximumPosition(int max);

  ///
  /// Utility function that set the minimum position and
  /// maximum position at once.
  void setPositions(int min, int max);
  
  ///
  /// When symmetricMoves is true, moving a handle will move the other handle
  /// symmetrically, otherwise the handles are independent. False by default
  bool symmetricMoves()const; 
  void setSymmetricMoves(bool symmetry);

  ///
  /// Controls the text to display for the handle tooltip. It is in addition
  /// to the widget tooltip.
  /// "%1" is replaced by the current value of the slider.
  /// Empty string (by default) means no tooltip.
  QString handleToolTip()const;
  void setHandleToolTip(const QString& toolTip);

  /// Returns true if the minimum value handle is down, false if it is up.
  /// \sa isMaximumSliderDown()
  bool isMinimumSliderDown()const;
  /// Returns true if the maximum value handle is down, false if it is up.
  /// \sa isMinimumSliderDown()
  bool isMaximumSliderDown()const;

Q_SIGNALS:
  ///
  /// This signal is emitted when the slider minimum value has changed, 
  /// with the new slider value as argument.
  void minimumValueChanged(int min);
  ///
  /// This signal is emitted when the slider maximum value has changed, 
  /// with the new slider value as argument.
  void maximumValueChanged(int max);
  ///
  /// Utility signal that is fired when minimum or maximum values have changed.
  void valuesChanged(int min, int max);

  ///
  /// This signal is emitted when sliderDown is true and the slider moves. 
  /// This usually happens when the user is dragging the minimum slider. 
  /// The value is the new slider minimum position.
  /// This signal is emitted even when tracking is turned off.
  void minimumPositionChanged(int min);

  ///
  /// This signal is emitted when sliderDown is true and the slider moves. 
  /// This usually happens when the user is dragging the maximum slider. 
  /// The value is the new slider maximum position.
  /// This signal is emitted even when tracking is turned off.
  void maximumPositionChanged(int max);

  ///
  /// Utility signal that is fired when minimum or maximum positions 
  /// have changed.
  void positionsChanged(int min, int max);

public Q_SLOTS:
  /// 
  /// This property holds the slider's current minimum value.
  /// The slider silently forces min to be within the legal range: 
  /// minimum() <= min <= maximumValue() <= maximum().
  /// Note: Changing the minimumValue also changes the minimumPosition.
  /// \sa stMaximumValue, setValues, setMinimum, setMaximum, setRange
  void setMinimumValue(int min);

  /// 
  /// This property holds the slider's current maximum value.
  /// The slider silently forces max to be within the legal range: 
  /// minimum() <= minimumValue() <= max <= maximum().
  /// Note: Changing the maximumValue also changes the maximumPosition.
  /// \sa stMinimumValue, setValues, setMinimum, setMaximum, setRange
  void setMaximumValue(int max);

  ///
  /// Utility function that set the minimum value and maximum value at once.
  /// The slider silently forces min and max to be within the legal range: 
  /// minimum() <= min <= max <= maximum().
  /// Note: Changing the minimumValue and maximumValue also changes the 
  /// minimumPosition and maximumPosition.
  /// \sa setMinimumValue, setMaximumValue, setMinimum, setMaximum, setRange
  void setValues(int min, int max);

protected Q_SLOTS:
  void onRangeChanged(int minimum, int maximum);

protected:
  ctkRangeSlider( ctkRangeSliderPrivate* impl, Qt::Orientation o, QWidget* par= 0 );
  ctkRangeSlider( ctkRangeSliderPrivate* impl, QWidget* par = 0 );

  // Description:
  // Standard Qt UI events
  virtual void mousePressEvent(QMouseEvent* ev);
  virtual void mouseMoveEvent(QMouseEvent* ev);
  virtual void mouseReleaseEvent(QMouseEvent* ev);

  // Description:
  // Rendering is done here.
  virtual void paintEvent(QPaintEvent* ev);
  virtual void initMinimumSliderStyleOption(QStyleOptionSlider* option) const;
  virtual void initMaximumSliderStyleOption(QStyleOptionSlider* option) const;

  // Description:
  // Reimplemented for the tooltips
  virtual bool event(QEvent* event);

protected:
  QScopedPointer<ctkRangeSliderPrivate> d_ptr;

private:
  Q_DECLARE_PRIVATE(ctkRangeSlider);
  Q_DISABLE_COPY(ctkRangeSlider);
};

#endif

