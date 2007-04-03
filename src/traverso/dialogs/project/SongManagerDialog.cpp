/*
Copyright (C) 2005-2006 Remon Sijrier 

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

#include "SongManagerDialog.h"

#include "libtraversocore.h"
#include <QStringList>
#include <QMessageBox>
#include <QFileDialog>
#include <QHeaderView>
#include <dialogs/NewSongDialog.h>
#include <Interface.h>

// Always put me below _all_ includes, this is needed
// in case we run with memory leak detection enabled!
#include "Debugger.h"

SongManagerDialog::SongManagerDialog( QWidget * parent )
	: QDialog(parent)
{
	setupUi(this);

	treeSongWidget->setColumnCount(3);
	treeSongWidget->header()->resizeSection(0, 160);
	treeSongWidget->header()->resizeSection(1, 55);
	treeSongWidget->header()->resizeSection(2, 70);
	QStringList stringList;
	stringList << "Song Name" << "Tracks" << "Length";
	treeSongWidget->setHeaderLabels(stringList);
	
	set_project(pm().get_project());
	
	connect(treeSongWidget, SIGNAL(itemClicked(QTreeWidgetItem*,int)), this, SLOT(songitem_clicked(QTreeWidgetItem*,int)));
	connect(&pm(), SIGNAL(projectLoaded(Project*)), this, SLOT(set_project(Project*)));
}

SongManagerDialog::~SongManagerDialog()
{}

void SongManagerDialog::set_project(Project* project)
{
	m_project = project;
	
	if (m_project) {
		connect(m_project, SIGNAL(songAdded(Song*)), this, SLOT(update_song_list()));
		connect(m_project, SIGNAL(songRemoved(Song*)), this, SLOT(update_song_list()));
		setWindowTitle("Manage Project - " + m_project->get_title());
	} else {
		setWindowTitle("Manage Project - No Project loaded!");
		treeSongWidget->clear();
	}
	
	update_song_list();
}


void SongManagerDialog::update_song_list( )
{
	if ( ! m_project) {
		printf("SongManagerDialog:: no project ?\n");
		return;
	}
	
	printf("update_song_list() \n");

	treeSongWidget->clear();
	foreach(Song* song, m_project->get_songs()) {

		QString songNr = QString::number(m_project->get_song_index(song->get_id()));
		QString songName = "Song " + songNr + " - " + song->get_title();
		QString numberOfTracks = QString::number(song->get_numtracks());
		QString songLength = frame_to_smpte(song->get_last_frame(), song->get_rate());
		QString songStatus = song->is_changed()?"UnSaved":"Saved";
		QString songSpaceAllocated = "Unknown";

		QTreeWidgetItem* item = new QTreeWidgetItem(treeSongWidget);
		item->setTextAlignment(1, Qt::AlignHCenter);
		item->setTextAlignment(2, Qt::AlignHCenter);
		item->setText(0, songName);
		item->setText(1, numberOfTracks);
		item->setText(2, songLength);
		
		item->setData(0, Qt::UserRole, song->get_id());
	}
}

void SongManagerDialog::songitem_clicked( QTreeWidgetItem* item, int)
{
	if (!item) {
		return;
	}

	Song* song;

	qint64 id = item->data(0, Qt::UserRole).toLongLong();
	song = m_project->get_song(id);

	Q_ASSERT(song);
		
	selectedSongName->setText(song->get_title());
	
	m_project->set_current_song(song->get_id());
}

void SongManagerDialog::on_renameSongButton_clicked( )
{
	if( ! m_project) {
		return;
	}
	
	QTreeWidgetItem* item = treeSongWidget->currentItem();
	
	
	
	if ( ! item) {
		return;
	}
	
	qint64 id = item->data(0, Qt::UserRole).toLongLong();
	Song* song = m_project->get_song(id);
	
	Q_ASSERT(song);
	
	QString newtitle = selectedSongName->text();
	
	if (newtitle.isEmpty()) {
		info().information(tr("No new Song name was supplied!"));
		return;
	}
	
	song->set_title(newtitle);

	update_song_list();
}

void SongManagerDialog::on_deleteSongButton_clicked( )
{
	QTreeWidgetItem* item = treeSongWidget->currentItem();
	
	if ( ! item ) {
		return;
	}
	
	qint64 id = item->data(0, Qt::UserRole).toLongLong();
	
	// Hmmm, to which history stack should this one be pushed ???? :-(
	Command::process_command(m_project->remove_song(m_project->get_song(id)));
}

void SongManagerDialog::on_createSongButton_clicked( )
{
	Interface::instance()->show_newsong_dialog();
}


void SongManagerDialog::hideEvent(QHideEvent * event)
{
	QDialog::hideEvent(event);
	
	if (m_project) {
		Song* song = m_project->get_current_song();
		if (song) {
			pm().get_undogroup()->setActiveStack(song->get_history_stack());
		}
	}
}

void SongManagerDialog::showEvent(QShowEvent * event)
{
	QDialog::showEvent(event);
	
	if (m_project) {
		pm().get_undogroup()->setActiveStack(m_project->get_history_stack());
	}
}

//eof

