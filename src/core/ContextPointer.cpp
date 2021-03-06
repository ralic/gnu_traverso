/*
Copyright (C) 2005-2010 Remon Sijrier

This file is part of Traverso

Traverso is free software; you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation; either version 2 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program; if not, write to the Free Software
Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA.

*/

#include "ContextPointer.h"

#include "ContextItem.h"
#include "TConfig.h"
#include "TInputEventDispatcher.h"
#include "TCommand.h"
#include <QCursor>


// Always put me below _all_ includes, this is needed
// in case we run with memory leak detection enabled!
#include "Debugger.h"


/**
 * \class ContextPointer
 * \brief ContextPointer forms the bridge between the ViewPort (GUI) and the InputEngine (core)
 *
	Use it in classes that inherit ViewPort to discover ViewItems under <br />
	the mouse cursor on the first input event x/y coordinates.<br />

	Also provides convenience functions to get ViewPort x/y coordinates<br />
	as well as scene x/y coordinates, which can be used for example in the <br />
	jog() implementation of Command classes.

	ViewPort's mouse event handling automatically updates the state of ContextPointer <br />
	as well as the InputEngine, which makes sure the mouse is grabbed and released <br />
	during Hold type Command's.

	Use cpointer() to get a reference to the singleton object!

 *	\sa ViewPort, InputEngine
 */


/**
 *
 * @return A reference to the singleton (static) ContextPointer object
 */
ContextPointer& cpointer()
{
	static ContextPointer contextPointer;
	return contextPointer;
}

ContextPointer::ContextPointer()
{
	m_x = 0;
	m_y = 0;
	m_jogEvent = false;
	m_port = 0;
	m_currentContext = 0;
	m_keyboardOnlyInput = false;

	m_mouseLeftClickBypassesJog = config().get_property("InputEventDispatcher", "mouseclicktakesoverkeyboardnavigation", false).toBool();

	connect(&m_jogTimer, SIGNAL(timeout()), this, SLOT(update_jog()));
	connect(&ied(), SIGNAL(jogStarted()), this, SLOT(jog_start()));
	connect(&ied(), SIGNAL(jogFinished()), this, SLOT(jog_finished()));
}

/**
 *  	Returns a list of all 'soft selected' ContextItems.

	To be able to also dispatch key facts to objects that
	don't inherit from ContextItem, but do inherit from
	QObject, the returned list holds QObjects.

 * @return A list of 'soft selected' ContextItems, as QObject's.
 */
QList< QObject * > ContextPointer::get_context_items( )
{
	PENTER;

	QList<QObject* > contextItems;
	ContextItem* item;
	ContextItem*  nextItem;

	QList<ContextItem*> activeItems;
	if (m_keyboardOnlyInput) {
		activeItems = m_activeContextItems;
	} else {
		activeItems = m_onFirstInputEventActiveContextItems;
	}

	for (int i=0; i < activeItems.size(); ++i) {
		item = activeItems.at(i);
		contextItems.append(item);
		while ((nextItem = item->get_context())) {
			contextItems.append(nextItem);
			item = nextItem;
		}
	}

	for (int i=0; i < m_contextItemsList.size(); ++i) {
		contextItems.append(m_contextItemsList.at(i));
	}


	return contextItems;
}

/**
 * 	Use this function to add an object that inherits from QObject <br />
	permanently to the 'soft selected' item list.

	The added object will always be added to the list returned in <br />
	get_context_items(). This way, one can add objects that do not <br />
	inherit ContextItem, to be processed into the key fact dispatching <br />
	of InputEngine.

 * @param item The QObject to be added to the 'soft selected' item list
 */
void ContextPointer::add_contextitem( QObject * item )
{
	if (! m_contextItemsList.contains(item))
		m_contextItemsList.append(item);
}

void ContextPointer::remove_contextitem(QObject* item)
{
	int index = m_contextItemsList.indexOf(item);
	m_contextItemsList.removeAt(index);
}

void ContextPointer::jog_start()
{
	m_globalMousePos = QCursor::pos();

	if (m_port) {
		m_port->grab_mouse();
	}
	m_jogEvent = true;
	int interval = config().get_property("InputEventDispatcher", "jogupdateinterval", 33).toInt();
	m_jogTimer.start(interval);
}

void ContextPointer::jog_finished()
{
	if (m_port) {
		emit contextChanged();
		m_port->release_mouse();
	}
	m_jogTimer.stop();
}

void ContextPointer::set_jog_bypass_distance(int distance)
{
	m_jogBypassDistance = distance;
}

void ContextPointer::set_left_mouse_click_bypasses_jog(bool bypassOnLeftMouseClick)
{
	m_mouseLeftClickBypassesJog = bypassOnLeftMouseClick;
}

void ContextPointer::mouse_button_left_pressed()
{
	if (m_mouseLeftClickBypassesJog) {
		set_keyboard_only_input(false);
	}
}

QList< QObject * > ContextPointer::get_contextmenu_items() const
{
	return m_contextMenuItems;
}

void ContextPointer::set_contextmenu_items(QList< QObject * > list)
{
	m_contextMenuItems = list;
}

void ContextPointer::update_jog()
{
	if (m_keyboardOnlyInput) {
		// no need or desire to call the current's
		// Hold Command::jog() function, were moving by keyboard now!
		return;
	}

	if (m_jogEvent) {
		ied().jog();
		m_jogEvent = false;
	}
}

void ContextPointer::set_current_viewport(AbstractViewPort *vp)
{
	PENTER;
	m_port = vp;
	if (!m_port) {
		m_onFirstInputEventActiveContextItems.clear();
		QList<ContextItem *> items;
		set_active_context_items(items);
	}
}

void ContextPointer::setCursorShape(const QString &cursor)
{
	if (!m_port)
	{
		return;
	}

	m_port->setCanvasCursorShape(cursor);
}

void ContextPointer::setCursorText(const QString &text, int mseconds)
{
	if (!m_port)
	{
		return;
	}

	m_port->setCursorText(text, mseconds);
}

void ContextPointer::setCursorPos(QPointF pos)
{
	if (!m_port)
	{
		return;
	}

	if (ied().get_holding_command() && ied().get_holding_command()->restoreCursorPosition())
	{
		QCursor::setPos(m_globalMousePos);
	}

	m_port->set_holdcursor_pos(pos);
}

void ContextPointer::store_canvas_cursor_position(int x, int y)
{
	m_x = x;
	m_y = y;
}

void ContextPointer::store_mouse_cursor_position(int x, int y)
{
	m_x = x;
	m_y = y;
	m_jogEvent = true;

	if (m_keyboardOnlyInput && !m_mouseLeftClickBypassesJog)
	{
		QPoint diff = m_globalMousePos - QCursor::pos();
		if (diff.manhattanLength() > m_jogBypassDistance)
		{
			set_keyboard_only_input(false);
		}
	}
}

void ContextPointer::set_active_context_items_by_mouse_movement(const QList<ContextItem *> &items)
{
	set_active_context_items(items);
}

void ContextPointer::set_active_context_items_by_keyboard_input(const QList<ContextItem *> &items)
{
	set_keyboard_only_input(true);
	m_globalMousePos = QCursor::pos();

	set_active_context_items(items);
}

void ContextPointer::set_active_context_items(const QList<ContextItem *> &items)
{
	foreach(ContextItem* oldItem, m_activeContextItems)
	{
		if (!items.contains(oldItem))
		{
			oldItem->set_has_active_context(false);
		}
	}

	m_activeContextItems.clear();

	foreach(ContextItem* item, items) {
		m_activeContextItems.append(item);
		item->set_has_active_context(true);
	}

	if (m_activeContextItems.isEmpty())
	{
		if (m_currentContext)
		{
			m_currentContext = 0;
			emit contextChanged();
		}
	}
	else if (m_activeContextItems.first() != m_currentContext)
	{
		m_currentContext = m_activeContextItems.first();
		emit contextChanged();
	}
}

void ContextPointer::remove_from_active_context_list(ContextItem *item)
{
	m_activeContextItems.removeAll(item);
	item->set_has_active_context(false);
}

void ContextPointer::about_to_delete(ContextItem *item)
{
	m_activeContextItems.removeAll(item);
	m_onFirstInputEventActiveContextItems.removeAll(item);
}

void ContextPointer::set_keyboard_only_input(bool keyboardOnly)
{
	PENTER;
	if (m_keyboardOnlyInput == keyboardOnly) {
		return;
	}

	m_keyboardOnlyInput = keyboardOnly;

	// Mouse cursor is taking over, let it look like it started
	// from the edit point :)
	if (!keyboardOnly)
	{
		QCursor::setPos(m_globalMousePos);
	}
}
