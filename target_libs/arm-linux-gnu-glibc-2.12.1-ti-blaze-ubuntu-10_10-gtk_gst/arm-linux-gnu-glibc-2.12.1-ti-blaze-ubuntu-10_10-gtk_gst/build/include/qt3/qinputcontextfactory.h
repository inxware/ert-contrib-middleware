/****************************************************************************
** $Id: qinputcontextfactory.h,v 1.1.1.1 2004/05/11 11:16:49 daisuke Exp $
**
** Definition of QInputContextFactory class
**
** Copyright (C) 2000-2002 Trolltech AS.  All rights reserved.
**
** This file is part of the widgets module of the Qt GUI Toolkit.
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

#ifndef QINPUTCONTEXTFACTORY_H
#define QINPUTCONTEXTFACTORY_H

#ifndef QT_H
#include "qstringlist.h"
#endif // QT_H

#ifndef QT_NO_IM

class QInputContext;
class QWidget;

class Q_EXPORT QInputContextFactory
{
public:
    static QStringList keys();
    static QInputContext *create( const QString &key, QWidget *widget ); // should be a toplevel widget
    static QStringList languages( const QString &key );
    static QString displayName( const QString &key );
    static QString description( const QString &key );
};
#endif //QT_NO_IM

#endif //QINPUTCONTEXTFACTORY_H