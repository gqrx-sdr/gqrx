//////////////////////////////////////////////////////////////////////
// freqctrl.h: interface for the CFreqCtrl class.
//
// History:
//  2010-09-15  Initial creation MSW
//  2011-03-27  Initial release
/////////////////////////////////////////////////////////////////////
#ifndef FREQCTRL_H
#define FREQCTRL_H
///////////////////////////////////////////////////////////////////////
// To use this control, add a frame using the QT designer editor.
//  Promote it to the CFreqCtrl class and include file freqctrl.h
// Initilaize the control in the constructor of the controls parent
//  ex: ui->frameFreqCtrl->Setup(9, 10000U, 230000000U, 1, UNITS_MHZ );
// where 9 is the number of display digits, min freq is 10kHz , Max is 230MHz
// the minimum step size is 1Hz and the freq is displayed as MHz
//    NOTE: the frequency is a qint64  64 bit integer value
//  to change frequency call SetFrequency()
//  ex:  ui->frameFreqCtrl->SetFrequency(146000000);
//
// One signal is sent when the control frequency changes:
//void NewFrequency(qint64 freq);   //emitted when frequency has changed
///////////////////////////////////////////////////////////////////////

#include <QtGui>
#include <QFrame>
#include <QImage>

enum FUNITS
{
    UNITS_HZ,
    UNITS_KHZ,
    UNITS_MHZ,
    UNITS_GHZ,
    UNITS_SEC,
    UNITS_MSEC,
    UNITS_USEC,
    UNITS_NSEC
};

#define MAX_DIGITS 12
#define MIN_DIGITS 4

class CFreqCtrl : public QFrame
{
    Q_OBJECT
public:
    explicit CFreqCtrl(QWidget *parent = 0);
    ~CFreqCtrl();
    QSize minimumSizeHint() const;
    QSize sizeHint() const;

    //primary access routines
    void setup(int NumDigits, qint64 Minf, qint64 Maxf,int MinStep, FUNITS UnitsType);
    void setUnits(FUNITS units);
    void setDigitColor(QColor cr);
    void setBkColor(QColor cr);
    void setUnitsColor(QColor cr);
    void setHighlightColor(QColor cr);
    qint64 getFrequency() { return m_freq; }

signals:
    void newFrequency(qint64 freq); //emitted when frequency has changed

public slots:
    void setFrequency(qint64 freq);

protected:      //overrides for this control
    void paintEvent(QPaintEvent *);
    void resizeEvent(QResizeEvent *);
    void mouseMoveEvent(QMouseEvent *);
    void mousePressEvent(QMouseEvent *);
    void wheelEvent(QWheelEvent *);
    void leaveEvent(QEvent *);
    void keyPressEvent(QKeyEvent *);

private:
    void updateCtrl(bool all);
    void drawBkGround(QPainter &Painter);
    void drawDigits(QPainter &Painter);
    void incDigit();
    void decDigit();
    void incFreq();
    void decFreq();
    void cursorHome();
    void cursorEnd();
    void moveCursorLeft();
    void moveCursorRight();
    bool inRect(QRect &rect, QPoint &point);

    bool m_UpdateAll;
    bool m_ExternalKeyActive;
    bool m_LRMouseFreqSel;    /*! Use left/right mouse buttons. If FALSE click area determines up/down. */

    bool m_ResetLowerDigits;  /*! If TRUE digits below the active one will be reset to 0
                                  when the active digit is incremented or decremented. */

    int m_FirstEditableDigit;
    int m_LastLeadZeroPos;
    int m_LeadZeroPos;
    int m_NumDigits;
    int m_DigStart;
    int m_ActiveEditDigit;
    int m_LastEditDigit;
    int m_DecPos;
    int m_NumSeps;

    qint64 m_MinStep;
    qint64 m_freq;
    qint64 m_Oldfreq;
    qint64 m_MinFreq;
    qint64 m_MaxFreq;

    QColor m_DigitColor;
    QColor m_BkColor;
    QColor m_UnitsColor;
    QColor m_HighlightColor;

    QPixmap m_Pixmap;
    QSize m_Size;
    FUNITS m_Units;

    QRect m_rectCtrl;               //main control rectangle
    QRect m_UnitsRect;              //rectangle where Units text goes
    QRect m_SepRect[MAX_DIGITS];    //separation rectangles for commas,dec pt, etc.

    QString m_UnitString;

    QFont m_DigitFont;
    QFont m_UnitsFont;

    struct DigStuct
    {
        qint64 weight;  //decimal weight of this digit
        qint64 incval;  //value this digit increments or decrements
        QRect dQRect;   //Digit bounding rectangle
        int val;    //value of this digit(0-9)
        bool modified;  //set if this digit has been modified
        bool editmode;  //set if this digit is selected for editing
    } m_DigitInfo[MAX_DIGITS];
};

#endif // FREQCTRL_H
