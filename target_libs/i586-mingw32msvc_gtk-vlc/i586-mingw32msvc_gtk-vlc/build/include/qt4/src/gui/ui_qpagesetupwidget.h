/********************************************************************************
** Form generated from reading UI file 'qpagesetupwidget.ui'
**
** Created: Tue 1. Dec 21:17:57 2009
**      by: Qt User Interface Compiler version 4.6.0
**
** WARNING! All changes made in this file will be lost when recompiling UI file!
********************************************************************************/

#ifndef UI_QPAGESETUPWIDGET_H
#define UI_QPAGESETUPWIDGET_H

#include <QtCore/QVariant>
#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QButtonGroup>
#include <QtGui/QComboBox>
#include <QtGui/QDoubleSpinBox>
#include <QtGui/QGridLayout>
#include <QtGui/QGroupBox>
#include <QtGui/QHBoxLayout>
#include <QtGui/QHeaderView>
#include <QtGui/QLabel>
#include <QtGui/QRadioButton>
#include <QtGui/QSpacerItem>
#include <QtGui/QVBoxLayout>
#include <QtGui/QWidget>

QT_BEGIN_NAMESPACE

class Ui_QPageSetupWidget
{
public:
    QGridLayout *gridLayout_3;
    QHBoxLayout *horizontalLayout_4;
    QComboBox *unit;
    QSpacerItem *horizontalSpacer_3;
    QGroupBox *groupBox_2;
    QGridLayout *gridLayout_2;
    QLabel *pageSizeLabel;
    QComboBox *paperSize;
    QLabel *widthLabel;
    QHBoxLayout *horizontalLayout_3;
    QDoubleSpinBox *paperWidth;
    QLabel *heightLabel;
    QDoubleSpinBox *paperHeight;
    QLabel *paperSourceLabel;
    QComboBox *paperSource;
    QSpacerItem *horizontalSpacer_4;
    QGroupBox *groupBox_3;
    QVBoxLayout *verticalLayout;
    QRadioButton *portrait;
    QRadioButton *landscape;
    QRadioButton *reverseLandscape;
    QRadioButton *reversePortrait;
    QSpacerItem *verticalSpacer_5;
    QWidget *preview;
    QGroupBox *groupBox;
    QHBoxLayout *horizontalLayout_2;
    QGridLayout *gridLayout;
    QDoubleSpinBox *topMargin;
    QHBoxLayout *horizontalLayout;
    QSpacerItem *horizontalSpacer_7;
    QDoubleSpinBox *leftMargin;
    QSpacerItem *horizontalSpacer;
    QDoubleSpinBox *rightMargin;
    QSpacerItem *horizontalSpacer_8;
    QDoubleSpinBox *bottomMargin;
    QSpacerItem *horizontalSpacer_2;
    QSpacerItem *horizontalSpacer_5;
    QSpacerItem *verticalSpacer;

    void setupUi(QWidget *QPageSetupWidget)
    {
        if (QPageSetupWidget->objectName().isEmpty())
            QPageSetupWidget->setObjectName(QString::fromUtf8("QPageSetupWidget"));
        QPageSetupWidget->resize(416, 488);
        gridLayout_3 = new QGridLayout(QPageSetupWidget);
        gridLayout_3->setContentsMargins(0, 0, 0, 0);
        gridLayout_3->setObjectName(QString::fromUtf8("gridLayout_3"));
        horizontalLayout_4 = new QHBoxLayout();
        horizontalLayout_4->setObjectName(QString::fromUtf8("horizontalLayout_4"));
        unit = new QComboBox(QPageSetupWidget);
        unit->setObjectName(QString::fromUtf8("unit"));

        horizontalLayout_4->addWidget(unit);

        horizontalSpacer_3 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout_4->addItem(horizontalSpacer_3);


        gridLayout_3->addLayout(horizontalLayout_4, 0, 0, 1, 2);

        groupBox_2 = new QGroupBox(QPageSetupWidget);
        groupBox_2->setObjectName(QString::fromUtf8("groupBox_2"));
        gridLayout_2 = new QGridLayout(groupBox_2);
        gridLayout_2->setObjectName(QString::fromUtf8("gridLayout_2"));
        pageSizeLabel = new QLabel(groupBox_2);
        pageSizeLabel->setObjectName(QString::fromUtf8("pageSizeLabel"));

        gridLayout_2->addWidget(pageSizeLabel, 0, 0, 1, 1);

        paperSize = new QComboBox(groupBox_2);
        paperSize->setObjectName(QString::fromUtf8("paperSize"));

        gridLayout_2->addWidget(paperSize, 0, 1, 1, 1);

        widthLabel = new QLabel(groupBox_2);
        widthLabel->setObjectName(QString::fromUtf8("widthLabel"));

        gridLayout_2->addWidget(widthLabel, 1, 0, 1, 1);

        horizontalLayout_3 = new QHBoxLayout();
        horizontalLayout_3->setObjectName(QString::fromUtf8("horizontalLayout_3"));
        paperWidth = new QDoubleSpinBox(groupBox_2);
        paperWidth->setObjectName(QString::fromUtf8("paperWidth"));
        paperWidth->setMaximum(9999.99);

        horizontalLayout_3->addWidget(paperWidth);

        heightLabel = new QLabel(groupBox_2);
        heightLabel->setObjectName(QString::fromUtf8("heightLabel"));

        horizontalLayout_3->addWidget(heightLabel);

        paperHeight = new QDoubleSpinBox(groupBox_2);
        paperHeight->setObjectName(QString::fromUtf8("paperHeight"));
        paperHeight->setMaximum(9999.99);

        horizontalLayout_3->addWidget(paperHeight);


        gridLayout_2->addLayout(horizontalLayout_3, 1, 1, 1, 1);

        paperSourceLabel = new QLabel(groupBox_2);
        paperSourceLabel->setObjectName(QString::fromUtf8("paperSourceLabel"));

        gridLayout_2->addWidget(paperSourceLabel, 2, 0, 1, 1);

        paperSource = new QComboBox(groupBox_2);
        paperSource->setObjectName(QString::fromUtf8("paperSource"));

        gridLayout_2->addWidget(paperSource, 2, 1, 1, 1);

        horizontalSpacer_4 = new QSpacerItem(40, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        gridLayout_2->addItem(horizontalSpacer_4, 1, 2, 1, 1);


        gridLayout_3->addWidget(groupBox_2, 1, 0, 1, 2);

        groupBox_3 = new QGroupBox(QPageSetupWidget);
        groupBox_3->setObjectName(QString::fromUtf8("groupBox_3"));
        verticalLayout = new QVBoxLayout(groupBox_3);
        verticalLayout->setObjectName(QString::fromUtf8("verticalLayout"));
        portrait = new QRadioButton(groupBox_3);
        portrait->setObjectName(QString::fromUtf8("portrait"));
        portrait->setChecked(true);

        verticalLayout->addWidget(portrait);

        landscape = new QRadioButton(groupBox_3);
        landscape->setObjectName(QString::fromUtf8("landscape"));

        verticalLayout->addWidget(landscape);

        reverseLandscape = new QRadioButton(groupBox_3);
        reverseLandscape->setObjectName(QString::fromUtf8("reverseLandscape"));

        verticalLayout->addWidget(reverseLandscape);

        reversePortrait = new QRadioButton(groupBox_3);
        reversePortrait->setObjectName(QString::fromUtf8("reversePortrait"));

        verticalLayout->addWidget(reversePortrait);

        verticalSpacer_5 = new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        verticalLayout->addItem(verticalSpacer_5);


        gridLayout_3->addWidget(groupBox_3, 2, 0, 1, 1);

        preview = new QWidget(QPageSetupWidget);
        preview->setObjectName(QString::fromUtf8("preview"));

        gridLayout_3->addWidget(preview, 2, 1, 2, 1);

        groupBox = new QGroupBox(QPageSetupWidget);
        groupBox->setObjectName(QString::fromUtf8("groupBox"));
        horizontalLayout_2 = new QHBoxLayout(groupBox);
        horizontalLayout_2->setObjectName(QString::fromUtf8("horizontalLayout_2"));
        gridLayout = new QGridLayout();
        gridLayout->setObjectName(QString::fromUtf8("gridLayout"));
        topMargin = new QDoubleSpinBox(groupBox);
        topMargin->setObjectName(QString::fromUtf8("topMargin"));
        topMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        topMargin->setMaximum(999.99);

        gridLayout->addWidget(topMargin, 0, 1, 1, 1);

        horizontalLayout = new QHBoxLayout();
        horizontalLayout->setObjectName(QString::fromUtf8("horizontalLayout"));
        horizontalSpacer_7 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_7);

        leftMargin = new QDoubleSpinBox(groupBox);
        leftMargin->setObjectName(QString::fromUtf8("leftMargin"));
        leftMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        leftMargin->setMaximum(999.99);

        horizontalLayout->addWidget(leftMargin);

        horizontalSpacer = new QSpacerItem(0, 0, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer);

        rightMargin = new QDoubleSpinBox(groupBox);
        rightMargin->setObjectName(QString::fromUtf8("rightMargin"));
        rightMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        rightMargin->setMaximum(999.99);

        horizontalLayout->addWidget(rightMargin);

        horizontalSpacer_8 = new QSpacerItem(0, 20, QSizePolicy::Expanding, QSizePolicy::Minimum);

        horizontalLayout->addItem(horizontalSpacer_8);


        gridLayout->addLayout(horizontalLayout, 1, 0, 1, 3);

        bottomMargin = new QDoubleSpinBox(groupBox);
        bottomMargin->setObjectName(QString::fromUtf8("bottomMargin"));
        bottomMargin->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
        bottomMargin->setMaximum(999.99);

        gridLayout->addWidget(bottomMargin, 2, 1, 1, 1);

        horizontalSpacer_2 = new QSpacerItem(0, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_2, 0, 2, 1, 1);

        horizontalSpacer_5 = new QSpacerItem(0, 20, QSizePolicy::MinimumExpanding, QSizePolicy::Minimum);

        gridLayout->addItem(horizontalSpacer_5, 0, 0, 1, 1);


        horizontalLayout_2->addLayout(gridLayout);


        gridLayout_3->addWidget(groupBox, 3, 0, 1, 1);

        verticalSpacer = new QSpacerItem(20, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);

        gridLayout_3->addItem(verticalSpacer, 4, 0, 1, 1);

#ifndef QT_NO_SHORTCUT
        pageSizeLabel->setBuddy(paperSize);
        widthLabel->setBuddy(paperWidth);
        heightLabel->setBuddy(paperHeight);
        paperSourceLabel->setBuddy(paperSource);
#endif // QT_NO_SHORTCUT

        retranslateUi(QPageSetupWidget);

        QMetaObject::connectSlotsByName(QPageSetupWidget);
    } // setupUi

    void retranslateUi(QWidget *QPageSetupWidget)
    {
        QPageSetupWidget->setWindowTitle(QApplication::translate("QPageSetupWidget", "Form", 0, QApplication::UnicodeUTF8));
        groupBox_2->setTitle(QApplication::translate("QPageSetupWidget", "Paper", 0, QApplication::UnicodeUTF8));
        pageSizeLabel->setText(QApplication::translate("QPageSetupWidget", "Page size:", 0, QApplication::UnicodeUTF8));
        widthLabel->setText(QApplication::translate("QPageSetupWidget", "Width:", 0, QApplication::UnicodeUTF8));
        heightLabel->setText(QApplication::translate("QPageSetupWidget", "Height:", 0, QApplication::UnicodeUTF8));
        paperSourceLabel->setText(QApplication::translate("QPageSetupWidget", "Paper source:", 0, QApplication::UnicodeUTF8));
        groupBox_3->setTitle(QApplication::translate("QPageSetupWidget", "Orientation", 0, QApplication::UnicodeUTF8));
        portrait->setText(QApplication::translate("QPageSetupWidget", "Portrait", 0, QApplication::UnicodeUTF8));
        landscape->setText(QApplication::translate("QPageSetupWidget", "Landscape", 0, QApplication::UnicodeUTF8));
        reverseLandscape->setText(QApplication::translate("QPageSetupWidget", "Reverse landscape", 0, QApplication::UnicodeUTF8));
        reversePortrait->setText(QApplication::translate("QPageSetupWidget", "Reverse portrait", 0, QApplication::UnicodeUTF8));
        groupBox->setTitle(QApplication::translate("QPageSetupWidget", "Margins", 0, QApplication::UnicodeUTF8));
#ifndef QT_NO_TOOLTIP
        topMargin->setToolTip(QApplication::translate("QPageSetupWidget", "top margin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_ACCESSIBILITY
        topMargin->setAccessibleName(QApplication::translate("QPageSetupWidget", "top margin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_ACCESSIBILITY
#ifndef QT_NO_TOOLTIP
        leftMargin->setToolTip(QApplication::translate("QPageSetupWidget", "left margin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_ACCESSIBILITY
        leftMargin->setAccessibleName(QApplication::translate("QPageSetupWidget", "left margin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_ACCESSIBILITY
#ifndef QT_NO_TOOLTIP
        rightMargin->setToolTip(QApplication::translate("QPageSetupWidget", "right margin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_ACCESSIBILITY
        rightMargin->setAccessibleName(QApplication::translate("QPageSetupWidget", "right margin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_ACCESSIBILITY
#ifndef QT_NO_TOOLTIP
        bottomMargin->setToolTip(QApplication::translate("QPageSetupWidget", "bottom margin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_TOOLTIP
#ifndef QT_NO_ACCESSIBILITY
        bottomMargin->setAccessibleName(QApplication::translate("QPageSetupWidget", "bottom margin", 0, QApplication::UnicodeUTF8));
#endif // QT_NO_ACCESSIBILITY
    } // retranslateUi

};

namespace Ui {
    class QPageSetupWidget: public Ui_QPageSetupWidget {};
} // namespace Ui

QT_END_NAMESPACE

#endif // UI_QPAGESETUPWIDGET_H
