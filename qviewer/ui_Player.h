/********************************************************************************
** Form generated from reading UI file 'Player.ui'
**
** Created: Sun Jan 20 12:10:41 2013
**      by: Qt User Interface Compiler version 4.8.3
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_PLAYER_H
#define UI_PLAYER_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QProgressBar>
#include <QtGui/QPushButton>
#include <QtGui/QSpinBox>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_PlayerDialog
{
public:
    QWidget *layoutWidget;
    QHBoxLayout *horizontalLayout;
    QProgressBar *frameProgressBar;
    QSpinBox *incrSpinBox;
    QWidget *layoutWidget1;
    QHBoxLayout *horizontalLayout_2;
    QPushButton *backwardButton;
    QPushButton *playButton;
    QPushButton *stopButton;
    QPushButton *forwardButton;
    QLabel *label;

    void setupUi(QWidget *PlayerDialog)
    {
        if (PlayerDialog->objectName().isEmpty())
            PlayerDialog->setObjectName(QString::fromUtf8("PlayerDialog"));
        PlayerDialog->resize(266, 78);
        layoutWidget = new QWidget(PlayerDialog);
        layoutWidget->setObjectName(QString::fromUtf8("layoutWidget"));
        layoutWidget->setGeometry(QRect(10, 40, 231, 29));
        horizontalLayout = new QHBoxLayout(layoutWidget);
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalLayout->setContentsMargins(0, 0, 0, 0);
        frameProgressBar = new QProgressBar(layoutWidget);
        frameProgressBar->setObjectName(QString::fromUtf8("frameProgressBar"));
        frameProgressBar->setValue(0);

        horizontalLayout->addWidget(frameProgressBar);

        incrSpinBox = new QSpinBox(layoutWidget);
        incrSpinBox->setObjectName(QString::fromUtf8("incrSpinBox"));
        incrSpinBox->setMinimum(1);
        incrSpinBox->setMaximum(50);

        horizontalLayout->addWidget(incrSpinBox);

        layoutWidget1 = new QWidget(PlayerDialog);
        layoutWidget1->setObjectName(QString::fromUtf8("layoutWidget1"));
        layoutWidget1->setGeometry(QRect(10, 10, 291, 32));
        horizontalLayout_2 = new QHBoxLayout(layoutWidget1);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        horizontalLayout_2->setContentsMargins(0, 0, 0, 0);
        backwardButton = new QPushButton(layoutWidget1);
        backwardButton->setObjectName(QString::fromUtf8("backwardButton"));
        QIcon icon;
        icon.addFile(QString::fromUtf8(":/icons/icons/rewind.xpm"), QSize(), QIcon::Normal, QIcon::Off);
        backwardButton->setIcon(icon);

        horizontalLayout_2->addWidget(backwardButton);

        playButton = new QPushButton(layoutWidget1);
        playButton->setObjectName(QString::fromUtf8("playButton"));
        QIcon icon1;
        icon1.addFile(QString::fromUtf8(":/icons/icons/pause.xpm"), QSize(), QIcon::Normal, QIcon::Off);
        playButton->setIcon(icon1);

        horizontalLayout_2->addWidget(playButton);

        stopButton = new QPushButton(layoutWidget1);
        stopButton->setObjectName(QString::fromUtf8("stopButton"));
        QIcon icon2;
        icon2.addFile(QString::fromUtf8(":/icons/icons/stop.xpm"), QSize(), QIcon::Normal, QIcon::Off);
        stopButton->setIcon(icon2);

        horizontalLayout_2->addWidget(stopButton);

        forwardButton = new QPushButton(layoutWidget1);
        forwardButton->setObjectName(QString::fromUtf8("forwardButton"));
        QIcon icon3;
        icon3.addFile(QString::fromUtf8(":/icons/icons/forward.xpm"), QSize(), QIcon::Normal, QIcon::Off);
        forwardButton->setIcon(icon3);

        horizontalLayout_2->addWidget(forwardButton);

        label = new QLabel(layoutWidget1);
        label->setObjectName(QString::fromUtf8("label"));

        horizontalLayout_2->addWidget(label);


        retranslateUi(PlayerDialog);

        QMetaObject::connectSlotsByName(PlayerDialog);
    } // setupUi

    void retranslateUi(QWidget *PlayerDialog)
    {
        PlayerDialog->setWindowTitle(QApplication::translate("PlayerDialog", "Player Controls", 0, QApplication::UnicodeUTF8));
        frameProgressBar->setFormat(QApplication::translate("PlayerDialog", "%v/%m", 0, QApplication::UnicodeUTF8));
        backwardButton->setText(QString());
        playButton->setText(QString());
        stopButton->setText(QString());
        forwardButton->setText(QString());
        label->setText(QApplication::translate("PlayerDialog", "Incr:", 0, QApplication::UnicodeUTF8));
    } // retranslateUi

};

namespace Ui {
    class PlayerDialog: public Ui_PlayerDialog {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_PLAYER_H
