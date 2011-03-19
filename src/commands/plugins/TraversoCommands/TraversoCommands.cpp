/*
Copyright (C) 2007-2010 Remon Sijrier

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

#include "TraversoCommands.h"

#include <libtraversocore.h>
#include <libtraversosheetcanvas.h>
#include <commands.h>
#include <float.h>
#include <QInputDialog>
#include "TMenuTranslator.h"
#include "TShortcutManager.h"

// Always put me below _all_ includes, this is needed
// in case we run with memory leak detection enabled!
#include "Debugger.h"

/**
 *	\class TraversoCommands
	\brief The Traverso CommandPlugin class which 'implements' many of the default Commands

	With this plugin, the InputEngine is able to dispatch key actions by directly
	asking this Plugin for the needed Command object.

	Dispatching key events to a specific object to create a Command object is accomplished by creating
	entries in the keymap.xml file, which has the following syntax:

	\code
	<Object objectname="" mousehint="" modes="" sortorder="" pluginname="" commandname="" arguments="" />
	\endcode

	\code
	objectname: 	The class name of the object that we want to dispatch the key action on
	mousehint 	Used to set the mouse cursor. Use "LR" for horizontal suggestion, "UD" for vertical suggestion, or "RLUD" for omnidirectional suggestion.
	modes:		Modes for which this command can be used, like All, Edit or Effects. Modes are to be defined in the keymap itself!
	sortorder:	The order of where the key action should show up in the ContextMenu of 'objectname'
	plugniname:	The plugin to ask the Command for, the only one available right now is TraversoCommands
	commandname:	The Command to be created, which by convention tries to be the same name as the actuall class implementing the Command
	arguments:	One, or more values (seperated by semicolons), each Command explains which (list of) values it can understand
	\endcode


	Available Command classes, their required Object (to operate on) and arguments are:


	\code
	MoveClip: Move or Copy-move an AudioClip.

	objectname:	AudioClipView
	arguments:	First entry, either one of the following: "move", "copy", "move_to_end", "move_to_start"
			Second (optional) entry: Move vertical only: "true" for vertical moving only, "false" for omnidirectional moving (the default if none is given)
	commandname:	MoveClip
	\endcode


	\code
	Gain: Change (jog), or reset the Gain of audio processing items
	objectname:	SheetView, TrackView, AudioClipView
	arguments:	No argument: Move Mouse vertically. Argument "horizontal": Move Mouse horizontally
	commandname:	Gain
	\endcode

	\code
	ResetGain: Set Gain value to a pre-defined value (by means of the supplied argument).

	objectname:	SheetView, TrackView, AudioClipView
	arguments:	a floating point value, eg: "0.5"
	commandname:	ResetGain
	\endcode

	TODO document all remaining Commands
	\code
	?: ?

	Objects(s):
	arguments:
	commandname:
	\endcode

 */


TraversoCommands::TraversoCommands()
{
	TFunction* function;

	function = new TFunction();
	function->object = "SheetView";
	function->description = tr("Arrow Key Browser: Up");
	function->commandName = "ArrowKeyBrowserUp";
	function->submenu = "Navigate";
	function->pluginname = "TraversoCommands";
	function->arguments << 1 << 100;
	addFunction(function, ArrowKeyBrowserCommand);

	function = new TFunction();
	function->object = "SheetView";
	function->description = tr("Arrow Key Browser: Down");
	function->commandName = "ArrowKeyBrowserDown";
	function->submenu = "Navigate";
	function->pluginname = "TraversoCommands";
	function->arguments << 2 << 100;
	addFunction(function, ArrowKeyBrowserCommand);

	function = new TFunction();
	function->object = "SheetView";
	function->description = tr("Arrow Key Browser: Left");
	function->commandName = "ArrowKeyBrowserLeft";
	function->submenu = "Navigate";
	function->pluginname = "TraversoCommands";
	function->arguments << 3 << 100;
	addFunction(function, ArrowKeyBrowserCommand);

	function = new TFunction();
	function->object = "SheetView";
	function->description = tr("Arrow Key Browser: Right");
	function->commandName = "ArrowKeyBrowserRight";
	function->submenu = "Navigate";
	function->arguments << 4 << 100;
	addFunction(function, ArrowKeyBrowserCommand);

	function = new TFunction();
	function->object = "AudioTrack";
	function->description = tr("Import Audio");
	function->commandName = "ImportAudio";
	addFunction(function, ImportAudioCommand);

	function = new TFunction();
	function->object = "AudioTrackView";
	function->description = tr("Fold Track");
	function->commandName = "FoldTrack";
	function->useX = true;
	function->arguments << "fold_track";
	addFunction(function, MoveClipCommand);


	function = new TFunction();
	function->object = "AudioClipView";
	function->description = tr("Move");
	function->commandName = "MoveClip";
	function->useX = function->useY = true;
	function->arguments << "move";
	addFunction(function, MoveClipCommand);

	function = new TFunction();
	function->object = "AudioClipView";
	function->description = tr("Copy");
	function->commandName = "CopyClip";
	function->useX = function->useY = true;
	function->arguments << "copy";
	addFunction(function, MoveClipCommand);

	function = new TFunction();
	function->object = "SheetView";
	function->description = tr("Fold Sheet");
	function->commandName = "FoldSheet";
	function->useX = function->useY = true;
	function->arguments << "fold_sheet";
	addFunction(function, MoveClipCommand);

	function = new TFunction();
	function->object = "TimeLineView";
	function->description = tr("Fold Markers");
	function->commandName = "FoldMarkers";
	function->useX = true;
	function->arguments << "fold_markers";
	addFunction(function, MoveClipCommand);

	function = new TFunction();
	function->object = "TrackView";
	function->description = tr("Move Up/Down");
	function->commandName = "MoveTrack";
	function->useY = true;
	addFunction(function, MoveTrackCommand);

	function = new TFunction();
	function->object = "CurveView";
	function->description = tr("Move Curve Node(s)");
	function->commandName = "MoveCurveNodes";
	function->useX = function->useY = true;
	addFunction(function, MoveCurveNodesCommand);

	function = new TFunction();
	function->object = "ProcessingData";
	function->description = tr("Gain");
	function->commandName = "Gain";
	addFunction(function, GainCommand);

	function = new TFunction();
	function->object = "SheetView";
	function->description = tr("Zoom");
	function->commandName = "Zoom";
	function->useX = true;
	function->arguments << "HJogZoom" << "1.2" << "0.2";
	addFunction(function, ZoomCommand);

	function = new TFunction();
	function->object = "AudioClipView";
	function->description = tr("Split");
	function->commandName = "SplitClip";
	function->useX = true;
	addFunction(function, SplitClipCommand);


	function = new TFunction();
	function->object = "TimeLineView";
	function->description = tr("Drag Marker");
	function->commandName = "TimeLineMoveMarker";
	function->useX = true;
	addFunction(function, MoveMarkerCommand);

	function = new TFunction();
	function->object = "MarkerView";
	function->description = tr("Drag Marker");
	function->commandName = "MoveMarker";
	function->useX = true;
	addFunction(function, MoveMarkerCommand);

//	<Object objectname="TimeLineView" mousehint="LR" slotsignature="drag_marker" sortorder="3" />
//	<Object objectname="MarkerView" mousehint="LR" slotsignature="drag_marker" sortorder="3" />



	// TODO:
	//<Object objectname="SheetView" mousehint="LR" sortorder="6" pluginname="TraversoCommands" commandname="Shuttle" />

}

void TraversoCommands::addFunction(TFunction *function, int command)
{
	function->pluginname = "TraversoCommands";
	function->commandName.prepend("TCP_");
	m_dict.insert(function->commandName, command);
	tShortCutManager().addFunction(function);
}

void TraversoCommands::create_menu_translations()
{
	TMenuTranslator* translator = TMenuTranslator::instance();

	translator->add_entry("TraversoCommands::Gain", tr("Gain"));
	translator->add_entry("TraversoCommands::ResetGain", tr("Gain: Reset"));
	translator->add_entry("TraversoCommands::TrackPan", tr("Panorama"));
	translator->add_entry("TraversoCommands::ResetTrackPan", tr("Panorama: Reset"));
	translator->add_entry("TraversoCommands::InsertSilence", tr("Insert Silence"));
	translator->add_entry("TraversoCommands::AddNewAudioTrack", tr("New Track"));
	translator->add_entry("TraversoCommands::RemoveClip", tr("Remove Clip"));
	translator->add_entry("TraversoCommands::RemoveTrack", tr("Remove"));
	translator->add_entry("TraversoCommands::AudioClipExternalProcessing", tr("External Processing"));
	translator->add_entry("TraversoCommands::ClipSelectionSelect", tr("(De)Select"));
	translator->add_entry("TraversoCommands::ClipSelectionSelectAll", tr("(De)Select All"));
	translator->add_entry("TraversoCommands::MoveEdge", tr("Move Edge"));
	translator->add_entry("TraversoCommands::MoveClipOrEdge", tr("Move Or Resize Clip"));
	translator->add_entry("TraversoCommands::CropClip", tr("Magnetic Cut"));
	translator->add_entry("TraversoCommands::ArmTracks", tr("Arm Tracks"));
	translator->add_entry("TraversoCommands::VZoomIn", tr("Vertical In"));
	translator->add_entry("TraversoCommands::HZoomOut", tr("Horizontal Out"));
	translator->add_entry("TraversoCommands::HZoomIn", tr("Horizontal In"));
	translator->add_entry("TraversoCommands::VZoomOut", tr("Vertical Out"));
	translator->add_entry("TraversoCommands::Zoom", tr("Zoom"));
	translator->add_entry("TraversoCommands::ScrollRightHold", tr("Right"));
	translator->add_entry("TraversoCommands::ScrollLeftHold", tr("Left"));
	translator->add_entry("TraversoCommands::ScrollUpHold", tr("Up"));
	translator->add_entry("TraversoCommands::ScrollDownHold", tr("Down"));
	translator->add_entry("TraversoCommands::Shuttle", tr("Shuttle"));
	translator->add_entry("TraversoCommands::NormalizeClip", tr("Normalize"));
	translator->add_entry("TraversoCommands::WorkCursorMove", tr("Move Work Cursor"));
	translator->add_entry("TraversoCommands::PlayHeadMove", tr("Set Play Position"));
}


TCommand* TraversoCommands::create(QObject* obj, const QString& command, QVariantList arguments)
{
	switch (m_dict.value(command)) {
		case GainCommand:
		{
			ContextItem* item = qobject_cast<ContextItem*>(obj);

			if (item->metaObject()->className() == QString("TrackPanelGain")) {
				item = item->get_context();
			} else if (AudioClipView* view = qobject_cast<AudioClipView*>(item)) {
				item = view->get_context();
			} else if (TrackView* view = qobject_cast<TrackView*>(item)) {
				item = view->get_context();
			}


			if (!item) {
				PERROR("TraversoCommands: Supplied QObject was not a ContextItem, "
					"GainCommand only works with ContextItem objects!!");
				return 0;
			}
			return new Gain(item, arguments);
		}

		case TrackPanCommand:
		{
			Track* track = qobject_cast<Track*>(obj);
			if (! track) {
				TPanKnobView* knob = qobject_cast<TPanKnobView*>(obj);
				if(knob) {
					track = knob->get_track();
				}
				if (!track) {
					PERROR("TraversoCommands: Supplied QObject was not a Track! "
						"TrackPanCommand needs a Track as argument");
					return 0;
				}
			}
			return new TrackPan(track, arguments);
		}

		case ImportAudioCommand:
		{
			AudioTrack* track = qobject_cast<AudioTrack*>(obj);
			if (! track) {
				PERROR("TraversoCommands: Supplied QObject was not a Track! "
					"ImportAudioCommand needs a Track as argument");
				return 0;
			}
			return new Import(track, TimeRef());
		}

		case InsertSilenceCommand:
		{
			AudioTrack* track = qobject_cast<AudioTrack*>(obj);
			if (! track) {
				PERROR("TraversoCommands: Supplied QObject was not a Track! "
					"ImportAudioCommand needs a Track as argument");
				return 0;
			}
			TimeRef length(10*UNIVERSAL_SAMPLE_RATE);
			return new Import(track, length, true);
		}

		case AddNewAudioTrackCommand:
		{
			Sheet* sheet = qobject_cast<Sheet*>(obj);
			if (!sheet) {
				PERROR("TraversoCommands: Supplied QObject was not a Sheet! "
					"AddNewAudioTrackCommand needs a Sheet as argument");
				return 0;
			}
			return sheet->add_track(new AudioTrack(sheet, "Unnamed", AudioTrack::INITIAL_HEIGHT));
		}

		case RemoveClipCommand:
		{
			AudioClip* clip = qobject_cast<AudioClip*>(obj);
			if (!clip) {
				PERROR("TraversoCommands: Supplied QObject was not a Clip! "
					"RemoveClipCommand needs a Clip as argument");
				return 0;
			}
			return new AddRemoveClip(clip, AddRemoveClip::REMOVE);
		}

		case RemoveTrackCommand:
		{
			Track* track = qobject_cast<Track*>(obj);
			if (!track) {
				PERROR("TraversoCommands: Supplied QObject was not a Track! "
					"RemoveTrackCommand needs a Track as argument");
				return 0;
			}

			TSession* activeSession = pm().get_project()->get_current_session();
			if (!activeSession) {
				// this is rather impossible!!
				info().information(tr("Removing Track %1, but no active (Work) Sheet ??").arg(track->get_name()));
				return 0;
			}

			if (track == activeSession->get_master_out()) {
				info().information(tr("It is not possible to remove the Master Out track!"));
				return 0;
			}
			return activeSession->remove_track(track);
		}

		case AudioClipExternalProcessingCommand:
		{
			AudioClip* clip = qobject_cast<AudioClip*>(obj);
			if (!clip) {
				PERROR("TraversoCommands: Supplied QObject was not an AudioClip! "
					"AudioClipExternalProcessingCommand needs an AudioClip as argument");
				return 0;
			}
			return new AudioClipExternalProcessing(clip);
		}

		case ClipSelectionCommand:
		{
			Sheet* sheet = qobject_cast<Sheet*>(obj);
			if (sheet) {
				QString action;
				if (arguments.size()) {
					action = arguments.at(0).toString();
					if (action == "select_all_clips") {
						return sheet->get_audioclip_manager()->select_all_clips();
					}
				}
			}
			AudioClip* clip = qobject_cast<AudioClip*>(obj);
			if (!clip) {
				PERROR("TraversoCommands: Supplied QObject was not an AudioClip! "
					"ClipSelectionCommand needs an AudioClip as argument");
				return 0;
			}

			// audio clip selection doesn't support/need number collection, but
			// other commands do, so if ie() has number collection, ignore it for this clip
			if (ie().has_collected_number()) {
				return ie().did_not_implement();
			}

			return new ClipSelection(clip, arguments);
		}

		case MoveClipCommand:
		{
			ViewItem* view = qobject_cast<ViewItem*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an AudioClipView! "
					"MoveClipCommand needs an AudioClipView as argument");
				return 0;
			}

			return new MoveClip(view, arguments);
		}

		case MoveTrackCommand:
		{
			TrackView* view = qobject_cast<TrackView*>(obj);

			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an TrackView! "
					"MoveTrackCommand needs an TrackView as argument");
				return 0;
			}

			return new MoveTrack(view);
		}


		case MoveEdgeCommand:
		{
			AudioClipView* view = qobject_cast<AudioClipView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an AudioClipView! "
					"MoveEdgeCommand needs an AudioClipView as argument");
				return 0;
			}

			int x = (int) (cpointer().on_first_input_event_scene_x() - view->scenePos().x());

			if (x < (view->boundingRect().width() / 2)) {
				return new MoveEdge(view, view->get_sheetview(), "set_left_edge");
			} else {
				return new MoveEdge(view, view->get_sheetview(), "set_right_edge");
			}
		}

		// The existence of this is doubtfull. Using [ E ] is so much easier
		// then trying to mimic 'if near to edge, drag edge' features.
		case MoveClipOrEdgeCommand:
		{
			AudioClipView* view = qobject_cast<AudioClipView*>(obj);

			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an AudioClipView! "
					"MoveClipOrEdgeCommand needs an AudioClipView as argument");
				return 0;
			}

			int x = (int) (cpointer().on_first_input_event_scene_x() - view->scenePos().x());

			int edge_width = 0;
			if (arguments.size() == 2) {
				edge_width = arguments[0].toInt();
			}

			if (x < edge_width) {
				return new MoveEdge(view, view->get_sheetview(), "set_left_edge");
			} else if (x > (view->boundingRect().width() - edge_width)) {
				return new MoveEdge(view, view->get_sheetview(), "set_right_edge");
			}

			return new MoveClip(view, QVariantList() << "move");
		}

		case SplitClipCommand:
		{
			AudioClipView* view = qobject_cast<AudioClipView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an AudioClipView! "
					"SplitClipCommand needs an AudioClipView as argument");
				return 0;
			}
			return new SplitClip(view);
		}

		case CropClipCommand:
		{
			AudioClipView* view = qobject_cast<AudioClipView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an AudioClipView! "
					"CropClipCommand needs an AudioClipView as argument");
				return 0;
			}
			return new CropClip(view);
		}

		case ArmTracksCommand:
		{
			SheetView* view = qobject_cast<SheetView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an SheetView! "
						"ArmTracksCommand needs an SheetView as argument");
				return 0;
			}
			return new ArmTracks(view);
		}

		case ZoomCommand:
		{
			SheetView* view = qobject_cast<SheetView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an SheetView! "
						"ZoomCommand needs an SheetView as argument");
				return 0;

			}
			return new Zoom(view, arguments);
		}

		case WorkCursorMoveCommand:
		{
			SheetView* view = qobject_cast<SheetView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an SheetView! "
						"WorkCursorMove Command needs an SheetView as argument");
				return 0;
			}
			return new WorkCursorMove(view);
		}

		case PlayHeadMoveCommand:
		{
			SheetView* view = qobject_cast<SheetView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an SheetView! "
						"WorkCursorMove Command needs an SheetView as argument");
				return 0;
			}
			return new PlayHeadMove(view);
		}

		case MoveCurveNodesCommand:
		{
			CurveView* curveView = qobject_cast<CurveView*>(obj);
			if (!curveView) {
				return 0;
			}

			return curveView->drag_node();
		}

		case ScrollCommand:
		{
			SheetView* view = qobject_cast<SheetView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an SheetView! "
						"ScrollCommand needs an SheetView as argument");
				return 0;
			}
			return new Scroll(view, arguments);
		}

		case ArrowKeyBrowserCommand:
		{
			SheetView* view = qobject_cast<SheetView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an SheetView! "
						"ScrollCommand needs an SheetView as argument");
				return 0;
			}
			return new ArrowKeyBrowser(view, arguments);
		}

		case ShuttleCommand:
		{
			SheetView* view = qobject_cast<SheetView*>(obj);
			if (!view) {
				PERROR("TraversoCommands: Supplied QObject was not an SheetView! "
						"ShuttleCommand needs an SheetView as argument");
				return 0;
			}
			return new Shuttle(view);
		}

		case NormalizeClipCommand:
		{
			AudioClip* clip = qobject_cast<AudioClip*>(obj);
			if (!clip) {
				PERROR("TraversoCommands: Supplied QObject was not a Clip! "
					"RemoveClipCommand needs a Clip as argument");
				return 0;
			}

			if (clip->is_selected()) {
				bool ok;
				float normfactor = FLT_MAX;

				double d = QInputDialog::getDouble(0, tr("Normalization"),
								   tr("Set Normalization level:"), 0.0, -120, 0, 1, &ok);

				if (!ok) {
					return 0;
				}
				QList<AudioClip* > selection;
				clip->get_sheet()->get_audioclip_manager()->get_selected_clips(selection);
				foreach(AudioClip* selected, selection) {
					normfactor = std::min(selected->calculate_normalization_factor(d), normfactor);
				}

				CommandGroup* group = new CommandGroup(clip, tr("Normalize Selected Clips"));

				foreach(AudioClip* selected, selection) {
					group->add_command(new PCommand(selected, "set_gain", normfactor, selected->get_gain(), tr("AudioClip: Normalize")));
				}

				return group;
			}

			return clip->normalize();
		}
		case MoveMarkerCommand:
		{
			TimeLineView* view = qobject_cast<TimeLineView*>(obj);
			if (view)
			{
				return view->drag_marker();
			}

			MarkerView* markerView = qobject_cast<MarkerView*>(obj);
			if (markerView)
			{
				return markerView->drag_marker();
			}

			PERROR("TraversoCommands: Supplied QObject was not a TimeLineView or MarkerView! "
				"MoveMarkerCommand needs a TimeLineView or MarkerView as argument");
			return 0;
		}

}

	return 0;
}


Q_EXPORT_PLUGIN2(tcp_traversocommands, TraversoCommands)

// eof
