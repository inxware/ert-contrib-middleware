/****************************************************************************
** $Id: qinputcontext.h,v 1.8 2004/06/22 06:47:30 daisuke Exp $
**
** Definition of QInputContext
**
** Copyright (C) 1992-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the kernel module of the Qt GUI Toolkit.
**
** This file may be distributed under the terms of the Q Public License
** as defined by Trolltech AS of Norway and appearing in the file
** LICENSE.QPL included in the packaging of this file.
**
** This file may be distributed and/or modified under the terms of the
** GNU General Public License version 2 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.
**
** Licensees holding valid Qt Enterprise Edition or Qt Professional Edition
** licenses may use this file in accordance with the Qt Commercial License
** Agreement provided with the Software.
**
** This file is provided AS IS with NO WARRANTY OF ANY KIND, INCLUDING THE
** WARRANTY OF DESIGN, MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
**
** See http://www.trolltech.com/pricing.html or email sales@trolltech.com for
**   information about Qt Commercial License Agreements.
** See http://www.trolltech.com/qpl/ for QPL licensing information.
** See http://www.trolltech.com/gpl/ for GPL licensing information.
**
** Contact info@trolltech.com if any conditions of this licensing are
** not clear to you.
**
**********************************************************************/

#ifndef QINPUTCONTEXT_H
#define QINPUTCONTEXT_H

#ifndef QT_NO_IM

#ifndef QT_H
#include "qobject.h"
#include "qglobal.h"
#include "qevent.h"
#include "qstring.h"
#if (QT_VERSION-0 >= 0x040000)
#include "qlist.h"
#include "qaction.h"
#else
#include "qptrlist.h"
#endif
#endif

class QWidget;
class QFont;
class QPopupMenu;
class QInputContextPrivate;


struct QInputContextMenu {
    enum Action {
	NoSeparator,
	InsertSeparator
    };
#if !(QT_VERSION-0 >= 0x040000)
    QString title;
    QPopupMenu *popup;
#endif
};


class QInputContext : public QObject
{
    Q_OBJECT
public:
    QInputContext( QObject *parent = 0 );
    virtual ~QInputContext();

    virtual QString identifierName();
    virtual QString language();

#if defined(Q_WS_X11)
    virtual bool x11FilterEvent( QWidget *keywidget, XEvent *event );
#endif // Q_WS_X11
    virtual bool filterEvent( const QEvent *event );
    virtual void reset();

    virtual void setFocus();
    virtual void unsetFocus();
    virtual void setMicroFocus( int x, int y, int w, int h, QFont *f = 0 );
    virtual void mouseHandler( int x, QEvent::Type type,
			       Qt::ButtonState button, Qt::ButtonState state );
    virtual QFont font() const;
    virtual bool isComposing() const;
    virtual bool isPreeditRelocationEnabled();

#if (QT_VERSION-0 >= 0x040000)
    virtual QList<QAction *> actions();
    void addActionsTo( QMenu *menu, QInputContextMenu::Action action = QInputContextMenu::InsertSeparator );
#else
    virtual QPtrList<QInputContextMenu> *menus();
    void addMenusTo( QPopupMenu *popup, QInputContextMenu::Action action = QInputContextMenu::InsertSeparator );
#endif

#if defined(Q_WS_X11)
    // these functions are not recommended for ordinary use 
    virtual QWidget *focusWidget() const;
    virtual QWidget *holderWidget() const;

    // these functions must not be used by ordinary input method
    virtual void setFocusWidget( QWidget *w );
    virtual void setHolderWidget( QWidget *w );
    virtual void releaseComposingWidget( QWidget *w );
#endif

signals:
    void deletionRequested();
    void imEventGenerated( QObject *receiver, QIMEvent *e );

protected:
    virtual void sendIMEvent( QEvent::Type type,
			      const QString &text = QString::null,
			      int cursorPosition = -1, int selLength = 0 );

private:
    void sendIMEventInternal( QEvent::Type type,
			      const QString &text = QString::null,
			      int cursorPosition = -1, int selLength = 0 );

    QInputContextPrivate *d;

    friend class QWidget;
    friend class QInputContextFactory;

private:   // Disabled copy constructor and operator=
    QInputContext( const QInputContext & );
    QInputContext &operator=( const QInputContext & );

};

#endif //Q_NO_IM

#endif // QINPUTCONTEXT_H
