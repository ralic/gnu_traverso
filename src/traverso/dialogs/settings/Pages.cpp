/*
Copyright (C) 2007 Remon Sijrier 

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

#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QMessageBox>
#include <QStyle>
#include <QStyleFactory>

#include "Pages.h"
#include <AudioDevice.h>
#if defined (ALSA_SUPPORT)
#include <AlsaDriver.h>
#endif
#include <Config.h>
#include <Utils.h>
#include <Themer.h>
#include <InputEngine.h>
#include "Interface.h"
#include <QDomDocument>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextEdit>

#if defined (JACK_SUPPORT)
RELAYTOOL_JACK
#endif


/****************************************/
/*            AudioDriver               */
/****************************************/


AudioDriverPage::AudioDriverPage(QWidget *parent)
    : ConfigPage(parent)
{
	// AudioDriverPage manages it's own layout
	// perhaps, should be done like the other pages as well
	// anyhow, the created layout should be deleted before
	// the new one is set.
	delete layout();
	
	
	QGroupBox* selectionGroup = new QGroupBox(tr("Driver Selection"));
	m_driverConfigPage = new DriverConfigPage(this);
	
	QLabel* driverLabel = new QLabel(tr("Driver:"));
	m_driverCombo = new QComboBox;
	
	QStringList drivers = audiodevice().get_available_drivers();
	foreach(const QString &name, drivers) {
		m_driverCombo->addItem(name);
	}
	
	QHBoxLayout* driverLayout = new QHBoxLayout;
	driverLayout->addWidget(driverLabel);
	driverLayout->addWidget(m_driverCombo);
	
	QVBoxLayout* selectionLayout = new QVBoxLayout;
	selectionLayout->addLayout(driverLayout);
	selectionGroup->setLayout(selectionLayout);

	QPushButton* restartDriverButton = new QPushButton(tr("Restart Driver"));
	QHBoxLayout* restartDriverButtonLayout = new QHBoxLayout;
	restartDriverButtonLayout->addStretch(1);
	restartDriverButtonLayout->addWidget(restartDriverButton);
	
	
	m_mainLayout = new QVBoxLayout;
	m_mainLayout->addWidget(selectionGroup);

#if defined (PORTAUDIO_SUPPORT)
	m_portaudiodrivers = new PaDriverPage(this);
	m_mainLayout->addWidget(m_portaudiodrivers);
#endif
	
	m_mainLayout->addWidget(m_driverConfigPage);
#if defined (ALSA_SUPPORT)
	m_alsadevices = new AlsaDevicesPage(this);
	m_mainLayout->addWidget(m_alsadevices);
#endif
	
	m_mainLayout->addLayout(restartDriverButtonLayout);
	m_mainLayout->addStretch(1);
	m_mainLayout->setSpacing(12);
	setLayout(m_mainLayout);
	
	connect(m_driverCombo, SIGNAL(currentIndexChanged(QString)), this, SLOT(driver_combobox_index_changed(QString)));
	connect(restartDriverButton, SIGNAL(clicked()), this, SLOT(restart_driver_button_clicked()));
	
	load_config();
}

void AudioDriverPage::save_config()
{
	config().set_property("Hardware", "samplerate", m_driverConfigPage->rateComboBox->currentText());
	int bufferindex = m_driverConfigPage->latencyComboBox->currentIndex();
	int buffersize = 1024;
	if (bufferindex >= 0) {
		buffersize = m_driverConfigPage->periodBufferSizesList.at(bufferindex);
	}
	config().set_property("Hardware", "buffersize", buffersize);
	
	config().set_property("Hardware", "drivertype", m_driverCombo->currentText());
	
	int playback=1, capture=1;
	if(m_driverConfigPage->duplexComboBox->currentIndex() == 1) {
		capture = 0;
	}
	
	if(m_driverConfigPage->duplexComboBox->currentIndex() == 2) {
		playback = 0;
	}
	
	config().set_property("Hardware", "capture", capture);
	config().set_property("Hardware", "playback", playback);

	
#if defined (ALSA_SUPPORT)
	int periods = m_alsadevices->periodsCombo->currentText().toInt();
	config().set_property("Hardware", "numberofperiods", periods);
	int index = m_alsadevices->devicesCombo->currentIndex();
	config().set_property("Hardware", "carddevice", m_alsadevices->devicesCombo->itemData(index));
	config().set_property("Hardware", "DitherShape", m_alsadevices->ditherShapeComboBox->currentText());
	
#endif

	
#if defined (PORTAUDIO_SUPPORT)
	int paindex = m_portaudiodrivers->driverCombo->currentIndex();
	config().set_property("Hardware", "pahostapi", m_portaudiodrivers->driverCombo->itemData(paindex));
#endif
	
	config().set_property("Hardware", "jackslave", m_driverConfigPage->jackTransportCheckBox->isChecked());
}

void AudioDriverPage::reset_default_config()
{
	config().set_property("Hardware", "samplerate", 44100);
	config().set_property("Hardware", "buffersize", 512);
#if defined (ALSA_SUPPORT)
	config().set_property("Hardware", "drivertype", "ALSA");
	config().set_property("Hardware", "carddevice", "hw:0");
	config().set_property("Hardware", "numberofperiods", 3);
	config().set_property("Hardware", "DitherShape", "None");
#elif defined (JACK_SUPPORT)
	if (libjack_is_present)
		config().set_property("Hardware", "drivertype", "Jack");
#else
	config().set_property("Hardware", "drivertype", "Null Driver");
#endif
	
#if defined (PORTAUDIO_SUPPORT)
#if defined (Q_WS_X11)
	config().set_property("Hardware", "pahostapi", "alsa");
#endif
#if defined (Q_WS_MAC)
	config().set_property("Hardware", "pahostapi", "coreaudio");
#endif
#if defined (Q_WS_WIN)
	config().set_property("Hardware", "pahostapi", "wmme");
#endif
#endif //end PORTAUDIO_SUPPORT
	
	config().set_property("Hardware", "capture", 1);
	config().set_property("Hardware", "playback", 1);
	
	config().set_property("Hardware", "jackslave", false);

	load_config();
}

void AudioDriverPage::load_config( )
{
	int samplerate = config().get_property("Hardware", "samplerate", 44100).toInt();
	int buffersize = config().get_property("Hardware", "buffersize", 512).toInt();
#if defined (Q_WS_X11)
	QString driverType = config().get_property("Hardware", "drivertype", "ALSA").toString();
#else
	QString driverType = config().get_property("Hardware", "drivertype", "PortAudio").toString();
#endif	
	bool capture = config().get_property("Hardware", "capture", 1).toInt();
	bool playback = config().get_property("Hardware", "playback", 1).toInt();


	int driverTypeIndex = m_driverCombo->findText(driverType);
	if (driverTypeIndex >= 0) {
		m_driverCombo->setCurrentIndex(driverTypeIndex);
	}
	
	driver_combobox_index_changed(driverType);
	
	int buffersizeIndex = m_driverConfigPage->periodBufferSizesList.indexOf(buffersize);
	int samplerateIndex = m_driverConfigPage->rateComboBox->findText(QString::number(samplerate));
	
	m_driverConfigPage->rateComboBox->setCurrentIndex(samplerateIndex);
	m_driverConfigPage->latencyComboBox->setCurrentIndex(buffersizeIndex);
	
	
	if (capture && playback) {
		m_driverConfigPage->duplexComboBox->setCurrentIndex(0);
	} else if (playback) {
		m_driverConfigPage->duplexComboBox->setCurrentIndex(1);
	} else {
		m_driverConfigPage->duplexComboBox->setCurrentIndex(2);
	}
	
	int index;
	
#if defined (ALSA_SUPPORT)
	m_alsadevices->devicesCombo->clear();
	int periodsIndex = config().get_property("Hardware", "numberofperiods", 3).toInt();
	QString ditherShape = config().get_property("Hardware", "DitherShape", "None").toString();
	m_alsadevices->periodsCombo->setCurrentIndex(periodsIndex - 2);
	
	int shapeIndex = m_alsadevices->ditherShapeComboBox->findText(ditherShape);
	if (shapeIndex >=0) {
		m_alsadevices->ditherShapeComboBox->setCurrentIndex(shapeIndex);
	}
	
	// Always add the 'default' device
	m_alsadevices->devicesCombo->addItem(tr("System default"), "default");
	
	// Iterate over the maximum number of devices that can be in a system
	// according to alsa, and add them to the devices list.
	QString name;
	for (int i=0; i<6; ++i) {
		name = AlsaDriver::alsa_device_name(false, i);
		if (name != "") {
			QString card = "Card " + QString::number(i+1) + ":  ";
			QString device = "hw:" + QString::number(i);
			m_alsadevices->devicesCombo->addItem(card + name, device);
		}
	}
	
	QString defaultdevice =  config().get_property("Hardware", "carddevice", "hw:0").toString();
	index = m_alsadevices->devicesCombo->findData(defaultdevice);
	if (index >= 0) {
		m_alsadevices->devicesCombo->setCurrentIndex(index);
	}
#endif
	
#if defined (PORTAUDIO_SUPPORT)
	m_portaudiodrivers->driverCombo->clear();
	QString defaulthostapi = "";

#if defined (Q_WS_X11)
	m_portaudiodrivers->driverCombo->addItem("ALSA", "alsa");
	m_portaudiodrivers->driverCombo->addItem("Jack", "jack");
	m_portaudiodrivers->driverCombo->addItem("OSS", "oss");
	defaulthostapi = "jack";
#endif

#if defined (Q_WS_MAC)
	m_portaudiodrivers->driverCombo->addItem("Core Audio", "coreaudio");
	m_portaudiodrivers->driverCombo->addItem("Jack", "jack");
	defaulthostapi = "coreaudio";
#endif

#if defined (Q_WS_WIN)
	m_portaudiodrivers->driverCombo->addItem("MME", "wmme");
	m_portaudiodrivers->driverCombo->addItem("Direct Sound", "directsound");
	m_portaudiodrivers->driverCombo->addItem("ASIO", "asio");
	defaulthostapi = "wmme";
#endif
	
	QString hostapi = config().get_property("Hardware", "pahostapi", defaulthostapi).toString();
	index = m_portaudiodrivers->driverCombo->findData(hostapi);
	if (index >= 0) {
		m_portaudiodrivers->driverCombo->setCurrentIndex(index);
	}
	
	m_driverConfigPage->update_latency_combobox();

#endif //end PORTAUDIO_SUPPORT

	bool usetransport = config().get_property("Hardware", "jackslave", false).toBool();
	m_driverConfigPage->jackTransportCheckBox->setChecked(usetransport);
}


void AudioDriverPage::restart_driver_button_clicked()
{
	QString driver = m_driverCombo->currentText();
	int rate = m_driverConfigPage->rateComboBox->currentText().toInt();
	int buffersize =  m_driverConfigPage->periodBufferSizesList.at(m_driverConfigPage->latencyComboBox->currentIndex());
	
	int playback=1, capture=1;
	if(m_driverConfigPage->duplexComboBox->currentIndex() == 1) {
		capture = 0;
	}
	
	if(m_driverConfigPage->duplexComboBox->currentIndex() == 2) {
		playback = 0;
	}
	

	QString cardDevice = "";
	QString dithershape = "None";


#if defined (ALSA_SUPPORT)
	int periods = m_alsadevices->periodsCombo->currentText().toInt();
	dithershape = m_alsadevices->ditherShapeComboBox->currentText();
	// The AlsaDriver retrieves it's periods number directly from config()
	// So there is no way to use the current selected one, other then
	// setting it now, and restoring it afterwards...
	int currentperiods = config().get_property("Hardware", "numberofperiods", 3).toInt();
	config().set_property("Hardware", "numberofperiods", periods);
	
	if (driver == "ALSA") {
		int index = m_alsadevices->devicesCombo->currentIndex();
		cardDevice = m_alsadevices->devicesCombo->itemData(index).toString();
	}
#endif
	
#if defined (PORTAUDIO_SUPPORT)
	if (driver == "PortAudio") {
		int index = m_portaudiodrivers->driverCombo->currentIndex();
		cardDevice = m_portaudiodrivers->driverCombo->itemData(index).toString();
	}
#endif
			
	audiodevice().set_parameters(rate, buffersize, driver, capture, playback, cardDevice, dithershape);
	
#if defined (ALSA_SUPPORT)
	config().set_property("Hardware", "numberofperiods", currentperiods);
#endif
	
	config().set_property("Hardware", "jackslave", m_driverConfigPage->jackTransportCheckBox->isChecked());
}



void AudioDriverPage::driver_combobox_index_changed(QString driver)
{
	if (driver == "ALSA" || driver == "PortAudio" || driver == "Null Driver") {
		m_driverConfigPage->driverConfigGroupBox->show();
	} else {
		m_driverConfigPage->driverConfigGroupBox->hide();
	}
	
#if defined (ALSA_SUPPORT)
	if (driver == "ALSA") {
		m_alsadevices->show();
		m_mainLayout->insertWidget(m_mainLayout->indexOf(m_driverConfigPage) + 1, m_alsadevices);
	} else {
		m_alsadevices->hide();
		m_mainLayout->removeWidget(m_alsadevices);
	}
#endif

#if defined (PORTAUDIO_SUPPORT)
	if (driver == "PortAudio") {
		m_portaudiodrivers->show();
		m_mainLayout->insertWidget(m_mainLayout->indexOf(m_driverConfigPage), m_portaudiodrivers);
	} else {
		m_portaudiodrivers->hide();
		m_mainLayout->removeWidget(m_portaudiodrivers);
	}
#endif
	
#if defined(JACK_SUPPORT)
	if (libjack_is_present && driver == "Jack") {
		m_driverConfigPage->jackGroupBox->show();
	} else {
		m_driverConfigPage->jackGroupBox->hide();
	}
#else
	m_driverConfigPage->jackGroupBox->hide();
#endif
}


DriverConfigPage::DriverConfigPage( QWidget * parent )
	: QWidget(parent)
{
	setupUi(this);
	periodBufferSizesList << 16 << 32 << 64 << 128 << 256 << 512 << 1024 << 2048 << 4096;
	
	connect(rateComboBox, SIGNAL(currentIndexChanged(QString)), this, SLOT(rate_combobox_index_changed(QString)));
}


void DriverConfigPage::update_latency_combobox( )
{
	latencyComboBox->clear();
	int rate = rateComboBox->currentText().toInt();
	int buffersize = audiodevice().get_buffer_size();
	
	for (int i=0; i<periodBufferSizesList.size(); ++i) {
		QString latency = QString::number( ((float)(periodBufferSizesList.at(i)) / rate) * 1000 * 2, 'f', 2);
		latencyComboBox->addItem(latency);
	}
	
	int index = periodBufferSizesList.indexOf(buffersize);
	latencyComboBox->setCurrentIndex(index);
}

void DriverConfigPage::rate_combobox_index_changed(QString )
{
	update_latency_combobox();
}

#if defined (ALSA_SUPPORT)
AlsaDevicesPage::AlsaDevicesPage(QWidget * parent)
	: QWidget(parent)
{
	setupUi(this);
}
#endif


#if defined (PORTAUDIO_SUPPORT)
PaDriverPage::PaDriverPage(QWidget * parent)
	: QWidget(parent)
{
	setupUi(this);
}
#endif


ConfigPage::ConfigPage(QWidget * parent)
	: QWidget(parent)
{
	mainLayout = new QVBoxLayout;
	setLayout(mainLayout);
}



/****************************************/
/*            Behavior                  */
/****************************************/


BehaviorPage::BehaviorPage(QWidget *parent)
	: ConfigPage(parent)
{
	m_configpage = new BehaviorConfigPage(this);
	mainLayout->addWidget(m_configpage);
	mainLayout->addStretch(1);
	connect(&config(), SIGNAL(configChanged()), this, SLOT(update_follow()));
	load_config();
}


void BehaviorPage::save_config()
{
	config().set_property("Project", "loadLastUsed", m_configpage->loadLastProjectCheckBox->isChecked());
	config().set_property("Sheet", "trackCreationCount", m_configpage->numberOfTrackSpinBox->value());
	config().set_property("PlayHead", "Follow", m_configpage->keepCursorVisibleCheckBox->isChecked());
	config().set_property("PlayHead", "Scrollmode", m_configpage->scrollModeComboBox->currentIndex());
	config().set_property("AudioClip", "SyncDuringDrag", m_configpage->resyncAudioCheckBox->isChecked());
	config().set_property("AudioClip", "LockByDefault", m_configpage->lockClipsCheckBox->isChecked());

	QString oncloseaction;
	if (m_configpage->saveRadioButton->isChecked()) {
		config().set_property("Project", "onclose", "save");
	} else if (m_configpage->askRadioButton->isChecked()) {
		config().set_property("Project", "onclose", "ask");
	} else {
		config().set_property("Project", "onclose", "dontsave");
	}
}

void BehaviorPage::load_config()
{
	bool loadLastUsedProject = config().get_property("Project", "loadLastUsed", 1).toBool();
	QString oncloseaction = config().get_property("Project", "onclose", "save").toString();
	int defaultNumTracks = config().get_property("Sheet", "trackCreationCount", 6).toInt();
	int scrollMode = config().get_property("PlayHead", "Scrollmode", 2).toInt();
	bool resyncAudio = config().get_property("AudioClip", "SyncDuringDrag", false).toBool();
	bool lockClips = config().get_property("AudioClip", "LockByDefault", false).toBool();
	QString interfaceLanguage = config().get_property("Interface", "language", "System Locale").toString();
	
	m_configpage->loadLastProjectCheckBox->setChecked(loadLastUsedProject);
	m_configpage->numberOfTrackSpinBox->setValue(defaultNumTracks);
	m_configpage->scrollModeComboBox->setCurrentIndex(scrollMode);
	m_configpage->resyncAudioCheckBox->setChecked(resyncAudio);
	m_configpage->lockClipsCheckBox->setChecked(lockClips);
	
	if (oncloseaction == "save") {
		m_configpage->saveRadioButton->setChecked(true);
	} else if (oncloseaction == "ask") {
		m_configpage->askRadioButton->setChecked(true);
	} else {
		m_configpage->neverRadioButton->setChecked(true);
	}
	
	update_follow();

}


void BehaviorPage::update_follow()
{
	bool keepCursorVisible = config().get_property("PlayHead", "Follow", true).toBool();
	m_configpage->keepCursorVisibleCheckBox->setChecked(keepCursorVisible);
	m_configpage->scrollModeComboBox->setEnabled(keepCursorVisible);
}

void BehaviorPage::reset_default_config()
{
	config().set_property("Project", "loadLastUsed", true);
	config().set_property("Project", "onclose", "save");
	config().set_property("Sheet", "trackCreationCount", 6);
	config().set_property("PlayHead", "Follow", 0);
	config().set_property("PlayHead", "Scrollmode", 2);
	config().set_property("AudioClip", "SyncDuringDrag", false);
	config().set_property("AudioClip", "LockByDefault", false);
	config().set_property("Interface", "language", "System Locale");
	
	load_config();
}


BehaviorConfigPage::BehaviorConfigPage(QWidget * parent)
	: QWidget(parent)
{
	setupUi(this);
}



/****************************************/
/*            Appearance                */
/****************************************/

AppearancePage::AppearancePage(QWidget *parent)
	: ConfigPage(parent)
{
	m_themepage = new ThemeConfigPage(this);
	mainLayout->addWidget(m_themepage);
	mainLayout->addStretch(1);
	
	load_config();
	m_themepage->create_connections();

	#if defined Q_WS_MAC
		m_themepage->iconSizeCombo->hide();
		m_themepage->toolbarStyleCombo->hide();
	#endif
}

void AppearancePage::save_config()
{
	config().set_property("Themer", "themepath", m_themepage->themePathLineEdit->text());
	config().set_property("Themer", "currenttheme", m_themepage->themeSelecterCombo->currentText());
	config().set_property("Themer", "coloradjust", m_themepage->colorAdjustBox->value());
	config().set_property("Themer", "style", m_themepage->styleCombo->currentText());
	config().set_property("Themer", "usestylepallet", m_themepage->useStylePalletCheckBox->isChecked());
	config().set_property("Themer", "paintaudiorectified", m_themepage->rectifiedCheckBox->isChecked());
	config().set_property("Themer", "paintstereoaudioasmono", m_themepage->mergedCheckBox->isChecked());
	config().set_property("Themer", "drawdbgrid", m_themepage->dbGridCheckBox->isChecked());
	config().set_property("Themer", "paintwavewithoutline", m_themepage->paintAudioWithOutlineCheckBox->isChecked());
	config().set_property("Themer", "iconsize", m_themepage->iconSizeCombo->currentText());
	config().set_property("Themer", "toolbuttonstyle", m_themepage->toolbarStyleCombo->currentIndex());
	config().set_property("Themer", "supportediconsizes", supportedIconSizes);
	config().set_property("Themer", "transportconsolesize", m_themepage->transportConsoleCombo->currentText());
}

void AppearancePage::load_config()
{
	QIcon icon = QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon);
	m_themepage->pathSelectButton->setIcon(icon);
	QString themepath = config().get_property("Themer", "themepath",
				   QString(QDir::homePath()).append(".traverso/themes")).toString();
	
	
	QStringList keys = QStyleFactory::keys();
	keys.sort();
	foreach(const QString &key, keys) {
		m_themepage->styleCombo->addItem(key);
	}
	
	m_themepage->update_theme_combobox(themepath);
	
	
	// Hmm, there seems no way to get the name of the current
	// used style, using the classname minus Q and Style seems to do the trick.
	QString systemstyle = QString(QApplication::style()->metaObject()->className()).remove("Q").remove("Style");
	QString style = config().get_property("Themer", "style", systemstyle).toString();
	QString theme  = config().get_property("Themer", "currenttheme", "TraversoLight").toString();
	int coloradjust = config().get_property("Themer", "coloradjust", 100).toInt();
	bool usestylepallete = config().get_property("Themer", "usestylepallet", "").toBool();
	bool paintRectified = config().get_property("Themer", "paintaudiorectified", false).toBool();
	bool paintStereoAsMono = config().get_property("Themer", "paintstereoaudioasmono", false).toBool();
	bool paintWaveWithLines = config().get_property("Themer", "paintwavewithoutline", true).toBool();
	bool dbGrid = config().get_property("Themer", "drawdbgrid", false).toBool();
	
	int index = m_themepage->styleCombo->findText(style);
	m_themepage->styleCombo->setCurrentIndex(index);
	index = m_themepage->themeSelecterCombo->findText(theme);
	m_themepage->themeSelecterCombo->setCurrentIndex(index);
	m_themepage->colorAdjustBox->setValue(coloradjust);
	m_themepage->useStylePalletCheckBox->setChecked(usestylepallete);
	m_themepage->themePathLineEdit->setText(themepath);
	m_themepage->rectifiedCheckBox->setChecked(paintRectified);
	m_themepage->mergedCheckBox->setChecked(paintStereoAsMono);
	m_themepage->dbGridCheckBox->setChecked(dbGrid);
	m_themepage->paintAudioWithOutlineCheckBox->setChecked(paintWaveWithLines);

	m_themepage->toolbarStyleCombo->clear();
	m_themepage->toolbarStyleCombo->addItem(tr("Icons only"));
	m_themepage->toolbarStyleCombo->addItem(tr("Text only"));
	m_themepage->toolbarStyleCombo->addItem(tr("Text beside Icons"));
	m_themepage->toolbarStyleCombo->addItem(tr("Text below Icons"));
	int tbstyle = config().get_property("Themer", "toolbuttonstyle", 0).toInt();
	m_themepage->toolbarStyleCombo->setCurrentIndex(tbstyle);

	// icon sizes of the toolbars
	QString iconsize = config().get_property("Themer", "iconsize", "22").toString();
	supportedIconSizes = config().get_property("Themer", "supportediconsizes", "16;22;32;48").toString();

	// if the list is empty, we should offer some default values. (The list can only be 
	// empty if someone deleted the values, but not the whole entry, in the config file.)
	if (supportedIconSizes.isEmpty()) {
		supportedIconSizes = "16;22;32;48";
	}

	QStringList iconSizesList = supportedIconSizes.split(";", QString::SkipEmptyParts);

	// check if the current icon size occurs in the list, if not, add it
	if (iconSizesList.lastIndexOf(iconsize) == -1) {
		iconSizesList << iconsize;
		iconSizesList.sort();
	}

	m_themepage->iconSizeCombo->clear();
	m_themepage->iconSizeCombo->addItems(iconSizesList);
	int iconsizeindex = m_themepage->iconSizeCombo->findText(iconsize);
	m_themepage->iconSizeCombo->setCurrentIndex(iconsizeindex);

	// and the same again for the icons size of the transport console
	QString trspsize = config().get_property("Themer", "transportconsolesize", "22").toString();
	iconSizesList = supportedIconSizes.split(";", QString::SkipEmptyParts);

	if (iconSizesList.lastIndexOf(iconsize) == -1) {
		iconSizesList << trspsize;
		iconSizesList.sort();
	}

	m_themepage->transportConsoleCombo->clear();
	m_themepage->transportConsoleCombo->addItems(iconSizesList);
	int trspsizeindex = m_themepage->iconSizeCombo->findText(trspsize);
	m_themepage->transportConsoleCombo->setCurrentIndex(trspsizeindex);
}

void AppearancePage::reset_default_config()
{
	m_themepage->styleCombo->clear();
	
	config().set_property("Themer", "themepath", QString(QDir::homePath()).append("/.traverso/themes"));
	config().set_property("Themer", "currenttheme", "TraversoLight");
	config().set_property("Themer", "coloradjust", 100);
	QString systemstyle = QString(QApplication::style()->metaObject()->className()).remove("Q").remove("Style");
	config().set_property("Themer", "style", systemstyle);
	config().set_property("Themer", "usestylepallet", false);
	config().set_property("Themer", "paintaudiorectified", false);
	config().set_property("Themer", "paintstereoaudioasmono", false);
	config().set_property("Themer", "drawdbgrid", false);
	config().set_property("Themer", "paintwavewithoutline", true);
	config().set_property("Themer", "supportediconsizes", "16;22;32;48");
	config().set_property("Themer", "iconsize", "22");
	config().set_property("Themer", "toolbuttonstyle", 0);

	load_config();
}


ThemeConfigPage::ThemeConfigPage(QWidget * parent)
	: QWidget(parent)
{
	setupUi(this);
	themeSelecterCombo->setInsertPolicy(QComboBox::InsertAlphabetically);
}

void ThemeConfigPage::create_connections()
{
	connect(styleCombo, SIGNAL(currentIndexChanged(const QString)), this, SLOT(style_index_changed(const QString)));
	connect(themeSelecterCombo, SIGNAL(currentIndexChanged(const QString)), this, SLOT(theme_index_changed(const QString)));
	connect(useStylePalletCheckBox, SIGNAL(toggled(bool)), this, SLOT(use_selected_styles_pallet_checkbox_toggled(bool)));
	connect(pathSelectButton, SIGNAL(clicked()), this, SLOT(dirselect_button_clicked()));
	connect(colorAdjustBox, SIGNAL(valueChanged(int)), this, SLOT(color_adjustbox_changed(int)));
	connect(rectifiedCheckBox, SIGNAL(toggled(bool)), this, SLOT(theme_option_changed()));
	connect(mergedCheckBox, SIGNAL(toggled(bool)), this, SLOT(theme_option_changed()));
	connect(dbGridCheckBox, SIGNAL(toggled(bool)), this, SLOT(theme_option_changed()));
	connect(paintAudioWithOutlineCheckBox, SIGNAL(toggled(bool)), this, SLOT(theme_option_changed()));
}

void ThemeConfigPage::style_index_changed(const QString& text)
{
	QApplication::setStyle(text);
	QIcon icon = QApplication::style()->standardIcon(QStyle::SP_DirClosedIcon);
	pathSelectButton->setIcon(icon);
	use_selected_styles_pallet_checkbox_toggled(useStylePalletCheckBox->isChecked());
}

void ThemeConfigPage::theme_index_changed(const QString & theme)
{
	int index = themeSelecterCombo->findText(theme);
	QString data = themeSelecterCombo->itemData(index).toString();
	QString path = config().get_property("Themer", "themepath", "").toString();
	
	if (data == "builtintheme") {
		themer()->use_builtin_theme(theme);
	} else {
		themer()->set_path_and_theme(path, theme);
	}
}

void ThemeConfigPage::use_selected_styles_pallet_checkbox_toggled(bool checked)
{
	if (checked) {
		QApplication::setPalette(QApplication::style()->standardPalette());
	} else {
		QApplication::setPalette(themer()->system_palette());
	}
}

void ThemeConfigPage::color_adjustbox_changed(int value)
{
	themer()->set_color_adjust_value(value);
}

void ThemeConfigPage::dirselect_button_clicked()
{
	QString path = themePathLineEdit->text();
	if (path.isEmpty()) {
		path = QDir::homePath();
	}
	QString dirName = QFileDialog::getExistingDirectory(this,
			tr("Select default project dir"), path);

	if (!dirName.isEmpty()) {
		themePathLineEdit->setText(dirName);
		update_theme_combobox(dirName);
	}
}

void ThemeConfigPage::update_theme_combobox(const QString& path)
{
	themeSelecterCombo->clear();
	
	foreach(QString key, themer()->get_builtin_themes()) {
		themeSelecterCombo->insertItem(0, key, "builtintheme");
	}
	
	QDir themedir(path);
	foreach (QString dirName, themedir.entryList(QDir::Dirs | QDir::NoDotAndDotDot)) {
		QString filename = path + "/" + dirName + "/" + "traversotheme.xml";
		if (QFile::exists(filename) ) {
			themeSelecterCombo->insertItem(0, dirName);
		}
	}
	
}

void ThemeConfigPage::theme_option_changed()
{
	config().set_property("Themer", "paintaudiorectified", rectifiedCheckBox->isChecked());
	config().set_property("Themer", "paintstereoaudioasmono", mergedCheckBox->isChecked());
	config().set_property("Themer", "drawdbgrid", dbGridCheckBox->isChecked());
	config().set_property("Themer", "paintwavewithoutline", paintAudioWithOutlineCheckBox->isChecked());
	themer()->load();
}


/****************************************/
/*            Keyboard                  */
/****************************************/

KeyboardPage::KeyboardPage(QWidget * parent)
	: ConfigPage(parent)
{
	m_configpage = new KeyboardConfigPage(this);
	mainLayout->addWidget(m_configpage);
	mainLayout->addStretch(5);
	
	load_config();
}

void KeyboardPage::load_config()
{
	int doubleFactTimeout = config().get_property("CCE", "doublefactTimeout", 180).toInt();
	int holdTimeout = config().get_property("CCE", "holdTimeout", 150).toInt();
	
	m_configpage->doubleFactTimeoutSpinBox->setValue(doubleFactTimeout);
	m_configpage->holdTimeoutSpinBox->setValue(holdTimeout);
	
	QString defaultkeymap = config().get_property("CCE", "keymap", "default").toString();
	int index = m_configpage->keymapComboBox->findText(defaultkeymap);
	if (index >= 0) {
		m_configpage->keymapComboBox->setCurrentIndex(index);
	}
}

void KeyboardPage::save_config()
{
	QString currentkeymap = config().get_property("CCE", "keymap", "default").toString();
	QString newkeymap = m_configpage->keymapComboBox->currentText();
	
	config().set_property("CCE", "doublefactTimeout", m_configpage->doubleFactTimeoutSpinBox->value());
	config().set_property("CCE", "holdTimeout", m_configpage->holdTimeoutSpinBox->value());
	config().set_property("CCE", "keymap", newkeymap);
	
	ie().set_double_fact_interval(m_configpage->doubleFactTimeoutSpinBox->value());
	ie().set_hold_sensitiveness(m_configpage->holdTimeoutSpinBox->value());
	if (currentkeymap != newkeymap) {
		ie().init_map(newkeymap);
	}
}

void KeyboardPage::reset_default_config()
{
	config().set_property("CCE", "doublefactTimeout", 180);
	config().set_property("CCE", "holdTimeout", 150);
	config().set_property("CCE", "keymap", "default");
	load_config();
}

KeyboardConfigPage::KeyboardConfigPage(QWidget * parent)
	: QWidget(parent)
{
	setupUi(this);
	connect(keymapComboBox, SIGNAL(currentIndexChanged(const QString)),
		this, SLOT(keymap_index_changed(const QString)));
	
	update_keymap_combo();
}

void KeyboardConfigPage::keymap_index_changed(const QString& keymap)
{
	QString filename = ":/keymaps/" + keymap + ".xml";
	if ( ! QFile::exists(filename)) {
		filename = QDir::homePath() + "/.traverso/keymaps/" + keymap + ".xml";
	}
	
	QDomDocument doc("keymap");
	QFile file(filename);
	if (!file.open(QIODevice::ReadOnly))
		return;
	if (!doc.setContent(&file)) {
		file.close();
		return;
	}
	file.close();

	QDomElement root = doc.documentElement();
	QDomNode mapinfo = root.firstChildElement("KeymapInfo");
	QDomElement e = mapinfo.toElement();
	QString description = e.attribute("description", tr("No description set for this keymap"));
	
	descriptionTextEdit->setHtml(description);
}

void KeyboardConfigPage::update_keymap_combo()
{
	keymapComboBox->clear();
	
	QDir keymapdir(":/keymaps");
	foreach (QString filename, keymapdir.entryList(QDir::Files)) {
		keymapComboBox->insertItem(0, filename.remove(".xml"));
	}
	
	keymapdir.setPath(QDir::homePath() + "/.traverso/keymaps");
	foreach (QString filename, keymapdir.entryList(QDir::Files)) {
		keymapComboBox->insertItem(0, filename.remove(".xml"));
	}
}

void KeyboardConfigPage::on_exportButton_clicked()
{
	Interface::instance()->export_keymap();
	QMessageBox::information( Interface::instance(), tr("KeyMap Export"), 
		     tr("The exported keymap can be found here:\n\n %1").arg(QDir::homePath() + "/traversokeymap.html"),
		     QMessageBox::Ok);
}

void KeyboardConfigPage::on_printButton_clicked()
{
	QString kmap;
	Interface::instance()->get_keymap(kmap);

	QPrinter printer(QPrinter::ScreenResolution);
	QPrintDialog printDialog(&printer, Interface::instance());
	if (printDialog.exec() == QDialog::Accepted) {
		QTextEdit edit;
		edit.insertHtml(kmap);
		edit.document()->print(&printer);
	}
}




PerformancePage::PerformancePage(QWidget * parent)
	: ConfigPage(parent)
{
	m_configpage = new PerformanceConfigPage(this);
	mainLayout->addWidget(m_configpage);
	mainLayout->addStretch(5);
	
	load_config();
}

void PerformancePage::load_config()
{
	int jogUpdateInterval = config().get_property("CCE", "jogupdateinterval", 28).toInt();
	bool useOpenGL = config().get_property("Interface", "OpenGL", false).toBool();
	
	m_configpage->jogUpdateIntervalSpinBox->setValue(1000 / jogUpdateInterval);
	m_configpage->useOpenGLCheckBox->setChecked(useOpenGL);	


	double buffertime = config().get_property("Hardware", "readbuffersize", 1.0).toDouble();
	m_configpage->bufferTimeSpinBox->setValue(buffertime);
}

void PerformancePage::save_config()
{
	config().set_property("Interface", "OpenGL", m_configpage->useOpenGLCheckBox->isChecked());
	config().set_property("CCE", "jogupdateinterval", 1000 / m_configpage->jogUpdateIntervalSpinBox->value());	
	double buffertime = m_configpage->bufferTimeSpinBox->value();
	config().set_property("Hardware", "readbuffersize", buffertime);
}

void PerformancePage::reset_default_config()
{
	config().set_property("CCE", "jogupdateinterval", 28);
	config().set_property("Interface", "OpenGL", false);
	config().set_property("Hardware", "readbuffersize", 1.0);
	load_config();
}

PerformanceConfigPage::PerformanceConfigPage(QWidget* parent)
	: QWidget(parent)
{
	setupUi(this);

#if defined (QT_OPENGL_SUPPORT)
	useOpenGLCheckBox->setEnabled(true);
#else
	useOpenGLCheckBox->setEnabled(false);
#endif
	QIcon icon = QApplication::style()->standardIcon(QStyle::SP_MessageBoxInformation);
	reloadWarningLabel->setPixmap(icon.pixmap(22, 22));
}

/****************************************/
/*            Recording                 */
/****************************************/


RecordingPage::RecordingPage(QWidget * parent)
	: ConfigPage(parent)
{
	m_config = new RecordingConfigPage(this);
	mainLayout->addWidget(m_config);
	mainLayout->addStretch(5);
	
	load_config();
}

void RecordingPage::load_config()
{
	bool useResampling = config().get_property("Conversion", "DynamicResampling", true).toBool();
	if (useResampling) {
		m_config->use_onthefly_resampling_checkbox_changed(Qt::Checked);
	} else {
		m_config->use_onthefly_resampling_checkbox_changed(Qt::Unchecked);
	}
	
	QString recordFormat = config().get_property("Recording", "FileFormat", "wav").toString();
	if (recordFormat == "wavpack") {
		m_config->encoding_index_changed(1);
	} else if (recordFormat == "w64") {
		m_config->encoding_index_changed(2);
	} else {
		m_config->encoding_index_changed(0);
	}
	
	QString wavpackcompression = config().get_property("Recording", "WavpackCompressionType", "fast").toString();
	if (wavpackcompression == "very_high") {
		m_config->wavpackCompressionComboBox->setCurrentIndex(0);
	} else if (wavpackcompression == "high") {
		m_config->wavpackCompressionComboBox->setCurrentIndex(1);
	} else {
		m_config->wavpackCompressionComboBox->setCurrentIndex(2);
	}		
	
	int index = config().get_property("Conversion", "RTResamplingConverterType", DEFAULT_RESAMPLE_QUALITY).toInt();
	m_config->ontheflyResampleComboBox->setCurrentIndex(index);
	
	index = config().get_property("Conversion", "ExportResamplingConverterType", 1).toInt();
	m_config->exportDefaultResampleQualityComboBox->setCurrentIndex(index);
}

void RecordingPage::save_config()
{
	config().set_property("Conversion", "DynamicResampling", m_config->useResamplingCheckBox->isChecked());
	config().set_property("Conversion", "RTResamplingConverterType", m_config->ontheflyResampleComboBox->currentIndex());
	config().set_property("Conversion", "ExportResamplingConverterType", m_config->exportDefaultResampleQualityComboBox->currentIndex());
	config().set_property("Recording", "FileFormat", m_config->encodingComboBox->itemData(m_config->encodingComboBox->currentIndex()).toString());
	config().set_property("Recording", "WavpackCompressionType", m_config->wavpackCompressionComboBox->itemData(m_config->wavpackCompressionComboBox->currentIndex()).toString());
	QString skipwvx = m_config->wavpackUseAlmostLosslessCheckBox->isChecked() ? "true" : "false";
	config().set_property("Recording", "WavpackSkipWVX", skipwvx);
}

void RecordingPage::reset_default_config()
{
	config().set_property("Conversion", "DynamicResampling", true);
	config().set_property("Conversion", "RTResamplingConverterType", DEFAULT_RESAMPLE_QUALITY);
	config().set_property("Conversion", "ExportResamplingConverterType", 1);
	config().set_property("Recording", "FileFormat", "wav");
	config().set_property("Recording", "WavpackCompressionType", "fast");
	config().set_property("Recording", "WavpackSkipWVX", "false");
	
	load_config();
}

RecordingConfigPage::RecordingConfigPage(QWidget * parent)
	: QWidget(parent)
{
	setupUi(this);
	
	encodingComboBox->addItem("WAV", "wav");
	encodingComboBox->addItem("WavPack", "wavpack");
	encodingComboBox->addItem("WAV64", "w64");
	wavpackCompressionComboBox->addItem("Very high", "very_high");
	wavpackCompressionComboBox->addItem("High", "high");
	wavpackCompressionComboBox->addItem("Fast", "fast");
	
	connect(encodingComboBox, SIGNAL(currentIndexChanged(int)), this, SLOT(encoding_index_changed(int)));
	connect(useResamplingCheckBox, SIGNAL(stateChanged(int)), 
		this, SLOT(use_onthefly_resampling_checkbox_changed(int)));
}

void RecordingConfigPage::encoding_index_changed(int index)
{
	encodingComboBox->setCurrentIndex(index);
	if (index != 1) {
		wacpackGroupBox->hide();
	} else {
		wacpackGroupBox->show();
	}
}

void RecordingConfigPage::use_onthefly_resampling_checkbox_changed(int state)
{
	if (state == Qt::Checked) {
		useResamplingCheckBox->setChecked(true);
		ontheflyResampleComboBox->setEnabled(true);
	} else {
		useResamplingCheckBox->setChecked(false);
		ontheflyResampleComboBox->setEnabled(false);
	}
}

