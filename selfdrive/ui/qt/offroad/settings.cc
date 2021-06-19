#include "settings.h"

#include <cassert>
#include <string>

#ifndef QCOM
#include "selfdrive/ui/qt/offroad/networking.h"
#endif
#include "selfdrive/common/params.h"
#include "selfdrive/common/util.h"
#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/qt/widgets/controls.h"
#include "selfdrive/ui/qt/widgets/input.h"
#include "selfdrive/ui/qt/widgets/offroad_alerts.h"
#include "selfdrive/ui/qt/widgets/scrollview.h"
#include "selfdrive/ui/qt/widgets/ssh_keys.h"
#include "selfdrive/ui/qt/widgets/toggle.h"
#include "selfdrive/ui/ui.h"
#include "selfdrive/ui/qt/util.h"

TogglesPanel::TogglesPanel(QWidget *parent) : QWidget(parent) {
  QVBoxLayout *toggles_list = new QVBoxLayout();

  QList<ParamControl*> toggles;

  toggles.append(new ParamControl("OpenpilotEnabledToggle",
                                  "Enable openpilot",
                                  "Use the openpilot system for adaptive cruise control and lane keep driver assistance. Your attention is required at all times to use this feature. Changing this setting takes effect when the car is powered off.",
                                  "../assets/offroad/icon_openpilot.png",
                                  this));
/*
  toggles.append(new ParamControl("IsRHD",
                                  "Enable Right-Hand Drive",
                                  "Allow openpilot to obey left-hand traffic conventions and perform driver monitoring on right driver seat.",
                                  "../assets/offroad/icon_openpilot_mirrored.png",
                                  this));
*/
  toggles.append(new ParamControl("IsMetric",
                                  "Use Metric System",
                                  "Display speed in km/h instead of mp/h.",
                                  "../assets/offroad/icon_metric.png",
                                  this));
  toggles.append(new ParamControl("CommunityFeaturesToggle",
                                  "Enable Community Features",
                                  "Use features from the open source community that are not maintained or supported by comma.ai and have not been confirmed to meet the standard safety model. These features include community supported cars and community supported hardware. Be extra cautious when using these features",
                                  "../assets/offroad/icon_discord.png",
                                  this));
  toggles.append(new ParamControl("IsLdwEnabled",
                                  "Enable Lane Departure Warnings",
                                  "Receive alerts to steer back into the lane when your vehicle drifts over a detected lane line without a turn signal activated while driving over 31mph (50kph).",
                                  "../assets/offroad/icon_ldws.png",
                                  this));
  toggles.append(new ParamControl("AutoLaneChangeEnabled",
                                  "Enable Auto Lane Change",
                                  "",
                                  "../assets/offroad/icon_lca.png",
                                  this));
/*
  toggles.append(new ParamControl("UploadRaw",
                                  "Upload Raw Logs",
                                  "Upload full logs and full resolution video by default while on WiFi. If not enabled, individual logs can be marked for upload at my.comma.ai/useradmin.",
                                  "../assets/offroad/icon_network.png",
                                  this));

  ParamControl *record_toggle = new ParamControl("RecordFront",
                                                 "Record and Upload Driver Camera",
                                                "Upload data from the driver facing camera and help improve the driver monitoring algorithm.",
                                                "../assets/offroad/icon_monitoring.png",
                                                this);
  toggles.append(record_toggle);
  toggles.append(new ParamControl("EndToEndToggle",
                                   "\U0001f96c Disable use of lanelines (Alpha) \U0001f96c",
                                   "In this mode openpilot will ignore lanelines and just drive how it thinks a human would.",
                                   "../assets/offroad/icon_road.png",
                                   this));

  if (Hardware::TICI()) {
    toggles.append(new ParamControl("EnableWideCamera",
                                    "Enable use of Wide Angle Camera",
                                    "Use wide angle camera for driving and ui.",
                                    "../assets/offroad/icon_openpilot.png",
                                    this));
    QObject::connect(toggles.back(), &ToggleControl::toggleFlipped, [=](bool state) {
      Params().remove("CalibrationParams");
    });

    toggles.append(new ParamControl("EnableLteOnroad",
                                    "Enable LTE while onroad",
                                    "",
                                    "../assets/offroad/icon_network.png",
                                    this));
  }
  bool record_lock = Params().getBool("RecordFrontLock");
  record_toggle->setEnabled(!record_lock);
*/
  for(ParamControl *toggle : toggles){
    if(toggles_list->count() != 0){
      toggles_list->addWidget(horizontal_line());
    }
    toggles_list->addWidget(toggle);
  }

  setLayout(toggles_list);
}

DevicePanel::DevicePanel(QWidget* parent) : QWidget(parent) {
  QVBoxLayout *device_layout = new QVBoxLayout;
  Params params = Params();

  QString dongle = QString::fromStdString(params.get("DongleId", false));
  device_layout->addWidget(new LabelControl("동글 ID", dongle));
/*
  device_layout->addWidget(horizontal_line());

  QString serial = QString::fromStdString(params.get("HardwareSerial", false));
  device_layout->addWidget(new LabelControl("Serial", serial));
*/
  // offroad-only buttons
  QList<ButtonControl*> offroad_btns;

  QString brand = params.getBool("Passive") ? "dashcam" : "openpilot";
  offroad_btns.append(new ButtonControl("Uninstall " + brand, "UNINSTALL", "", [=]() {
    if (ConfirmationDialog::confirm("Are you sure you want to uninstall?", this)) {
      Params().putBool("DoUninstall", true);
    }
  }, "", this));
  offroad_btns.append(new ButtonControl("Review Training Guide", "REVIEW",
                                        "Review the rules, features, and limitations of openpilot", [=]() {
    if (ConfirmationDialog::confirm("Are you sure you want to review the training guide?", this)) {
      Params().remove("CompletedTrainingVersion");
      emit reviewTrainingGuide();
    }
  }, "", this));

  offroad_btns.append(new ButtonControl("Driver Camera", "PREVIEW",
                                        "Preview the driver facing camera to help optimize device mounting position for best driver monitoring experience. (vehicle must be off)",
                                        [=]() { emit showDriverView(); }, "", this));

  QString resetCalibDesc = "(pitch) ↕ 5˚ \n(yaw) ↔ 4˚";

  ButtonControl *resetCalibBtn = new ButtonControl("Reset Calibration", "RESET", resetCalibDesc, [=]() {
    QString desc = resetCalibDesc;
    std::string calib_bytes = Params().get("CalibrationParams");
    if (!calib_bytes.empty()) {
      try {
        AlignedBuffer aligned_buf;
        capnp::FlatArrayMessageReader cmsg(aligned_buf.align(calib_bytes.data(), calib_bytes.size()));
        auto calib = cmsg.getRoot<cereal::Event>().getLiveCalibration();
        if (calib.getCalStatus() != 0) {
          double pitch = calib.getRpyCalib()[1] * (180 / M_PI);
          double yaw = calib.getRpyCalib()[2] * (180 / M_PI);
          desc += QString(" Your device is pointed %1° %2 and %3° %4.")
                                .arg(QString::number(std::abs(pitch), 'g', 1), pitch > 0 ? "↑" : "↓",
                                     QString::number(std::abs(yaw), 'g', 1), yaw > 0 ? "→" : "←");
        }
      } catch (kj::Exception) {
        qInfo() << "invalid CalibrationParams";
      }
    }
    if (ConfirmationDialog::confirm(desc)) {
      //Params().remove("CalibrationParams");
    }
  }, "", this);
  connect(resetCalibBtn, &ButtonControl::showDescription, [=]() {
    QString desc = resetCalibDesc;
    std::string calib_bytes = Params().get("CalibrationParams");
    if (!calib_bytes.empty()) {
      try {
        AlignedBuffer aligned_buf;
        capnp::FlatArrayMessageReader cmsg(aligned_buf.align(calib_bytes.data(), calib_bytes.size()));
        auto calib = cmsg.getRoot<cereal::Event>().getLiveCalibration();
        if (calib.getCalStatus() != 0) {
          double pitch = calib.getRpyCalib()[1] * (180 / M_PI);
          double yaw = calib.getRpyCalib()[2] * (180 / M_PI);
          desc += QString(" Your device is pointed %1° %2 and %3° %4.")
                                .arg(QString::number(std::abs(pitch), 'g', 1), pitch > 0 ? "↑" : "↓",
                                     QString::number(std::abs(yaw), 'g', 1), yaw > 0 ? "→" : "←");
        }
      } catch (kj::Exception) {
        qInfo() << "invalid CalibrationParams";
      }
    }
    resetCalibBtn->setDescription(desc);
  });

  offroad_btns.append(resetCalibBtn);

  for(auto &btn : offroad_btns){
    //device_layout->addWidget(horizontal_line());
    QObject::connect(parent, SIGNAL(offroadTransition(bool)), btn, SLOT(setEnabled(bool)));
    device_layout->addWidget(btn);
  }

  // cal reset and param init buttons
  QHBoxLayout *cal_param_init_layout = new QHBoxLayout();
  cal_param_init_layout->setSpacing(30);

  QPushButton *calinit_btn = new QPushButton("Calibration Init");
  cal_param_init_layout->addWidget(calinit_btn);
  QObject::connect(calinit_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("Process?", this)) {
      Params().remove("CalibrationParams");
    }
  });

  QPushButton *calinit_btn2 = new QPushButton("Calibration Init/Reboot");
  cal_param_init_layout->addWidget(calinit_btn2);
  QObject::connect(calinit_btn2, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("Process?", this)) {
      Params().remove("CalibrationParams");
      Hardware::reboot();
    }
  });

  // power buttons
  QHBoxLayout *power_layout = new QHBoxLayout();
  power_layout->setSpacing(30);

  QPushButton *reboot_btn = new QPushButton("Reboot");
  reboot_btn->setStyleSheet("color: white;"
                            "background-color: #2CE22C;");
  power_layout->addWidget(reboot_btn);
  QObject::connect(reboot_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("Are you sure you want to reboot?", this)) {
      Hardware::reboot();
    }
  });

  QPushButton *poweroff_btn = new QPushButton("Power Off");
  poweroff_btn->setStyleSheet("background-color: #E22C2C;");
  power_layout->addWidget(poweroff_btn);
  QObject::connect(poweroff_btn, &QPushButton::released, [=]() {
    if (ConfirmationDialog::confirm("Are you sure you want to power off?", this)) {
      Hardware::poweroff();
    }
  });
  device_layout->addLayout(cal_param_init_layout);
  device_layout->addLayout(power_layout);

  setLayout(device_layout);
  setStyleSheet(R"(
    QPushButton {
      font-weight: 500;
      padding: 0;
      height: 120px;
      border-radius: 15px;
      background-color: #393939;
    }
  )");
}

SoftwarePanel::SoftwarePanel(QWidget* parent) : QFrame(parent) {
  QVBoxLayout *main_layout = new QVBoxLayout(this);
  setLayout(main_layout);
  setStyleSheet(R"(QLabel {font-size: 50px;})");

  fs_watch = new QFileSystemWatcher(this);
  QObject::connect(fs_watch, &QFileSystemWatcher::fileChanged, [=](const QString path) {
    int update_failed_count = Params().get<int>("UpdateFailedCount").value_or(0);
    if (path.contains("UpdateFailedCount") && update_failed_count > 0) {
      lastUpdateTimeLbl->setText("failed to fetch update");
      updateButton->setText("CHECK");
      updateButton->setEnabled(true);
    } else if (path.contains("LastUpdateTime")) {
      updateLabels();
    }
  });
}

void SoftwarePanel::showEvent(QShowEvent *event) {
  updateLabels();
}

void SoftwarePanel::updateLabels() {
  Params params = Params();
  std::string brand = params.getBool("Passive") ? "dashcam" : "openpilot";
  /*
  QList<QPair<QString, std::string>> dev_params = {
    {"Version", "v" + params.get("Version")},
    {"Git Remote", params.get("GitRemote").substr(19)},
    {"Git Branch", params.get("GitBranch")},
    {"Git Commit", params.get("GitCommit").substr(0, 7)},
    {"Panda Firmware", params.get("PandaFirmwareHex")},
    {"OS Version", Hardware::get_os_version()},
  };

  QString lastUpdateTime = "";

  std::string last_update_param = params.get("LastUpdateTime");
  if (!last_update_param.empty()){
    QDateTime lastUpdateDate = QDateTime::fromString(QString::fromStdString(last_update_param + "Z"), Qt::ISODate);
    lastUpdateTime = timeAgo(lastUpdateDate);
  }

  if (true) {

    lastUpdateTimeLbl = new LabelControl("최종 업데이트", lastUpdateTime, "");
    layout()->addWidget(lastUpdateTimeLbl);

    updateButton = new ButtonControl("업데이트 확인", "확인", "", [=]() {
      Params params = Params();
      if (params.getBool("IsOffroad")) {
        fs_watch->addPath(QString::fromStdString(params.getParamsPath()) + "/d/LastUpdateTime");
        fs_watch->addPath(QString::fromStdString(params.getParamsPath()) + "/d/UpdateFailedCount");
        updateButton->setText("확인중");
        updateButton->setEnabled(false);
      }
      std::system("pkill -1 -f selfdrive.updated");
    }, "", this);
    layout()->addWidget(updateButton);
*/
  QString version = QString::fromStdString("v" + params.get("Version"));
  versionLbl = new LabelControl("Version", version, "");
  layout()->addWidget(versionLbl);

  QString neos = QString::fromStdString(Hardware::get_os_version());
  neosLbl = new LabelControl("NEOS", neos, "");
  layout()->addWidget(neosLbl);

  layout()->addWidget(horizontal_line());
  QString remote = QString::fromStdString(params.get("GitRemote").substr(19));
  remoteLbl = new LabelControl("Git Remote", remote, "");
  layout()->addWidget(remoteLbl);

  QString branch = QString::fromStdString(params.get("GitBranch"));
  branchLbl = new LabelControl("Git Branch", branch, "");
  layout()->addWidget(branchLbl);

  QString commit = QString::fromStdString(params.get("GitCommit").substr(0, 7));
  commitLbl = new LabelControl("Git Commit", commit, "");
  layout()->addWidget(commitLbl);

  const char* gitpull = "/data/openpilot/gitpull.sh ''";
  layout()->addWidget(new ButtonControl("Git Pull", "Run", "", [=]() {
                                        if (ConfirmationDialog::confirm("Process?")){
                                          std::system(gitpull);
                                        }
                                      }));
  layout()->addWidget(horizontal_line());
  const char* panda_flash = "/data/openpilot/panda/board/flash.sh ''";
  layout()->addWidget(new ButtonControl("Panda Firmware Flash", "Run", "", [=]() {
                                        if (ConfirmationDialog::confirm("Process?")){
                                          std::system(panda_flash);
                                        }
                                      }));
  const char* panda_recover = "/data/openpilot/panda/board/recover.sh ''";
  layout()->addWidget(new ButtonControl("Panda Firmware Recover", "Run", "", [=]() {
                                        if (ConfirmationDialog::confirm("Process?")){
                                          std::system(panda_recover);
                                        }
                                      }));
  }

/*
  for (int i = 0; i < dev_params.size(); i++) {
    const auto &[name, value] = dev_params[i];
    QString val = QString::fromStdString(value).trimmed();
    if (labels.size() > i) {
      labels[i]->setText(val);
    } else {
      labels.push_back(new LabelControl(name, val));
      layout()->addWidget(labels[i]);
      if (i < (dev_params.size() - 1)) {
        layout()->addWidget(horizontal_line());
      }
    }
  }
}
*/

QWidget * network_panel(QWidget * parent) {
  QVBoxLayout *layout = new QVBoxLayout;
  layout->setSpacing(30);
#ifdef QCOM
  // wifi + tethering buttons
  layout->addWidget(new ButtonControl("\U0001f4f6 WiFi Settings", "OPEN", "",
                                      [=]() { HardwareEon::launch_wifi(); }));
  // Android Setting
  layout->addWidget(new ButtonControl("\U00002699 Android Settings", "OPEN", "",
                                      [=]() { HardwareEon::launch_setting(); }));
  layout->addWidget(horizontal_line());
#endif
/*
  layout->addWidget(new ButtonControl("Tethering Settings", "OPEN", "",
                                      [=]() { HardwareEon::launch_tethering(); }));
  layout->addWidget(horizontal_line());
*/
  // SSH key management
  layout->addWidget(new SshToggle());
  layout->addWidget(new SshControl());
  layout->addWidget(horizontal_line());
  layout->addWidget(new LateralControlSelect());
  layout->addWidget(new MfcSelect());
  layout->addWidget(new LongControlSelect());
  layout->addWidget(horizontal_line());
  layout->addWidget(new PrebuiltToggle());
  layout->addWidget(new ShutdowndToggle());
  layout->addWidget(new LoggerToggle());
  layout->addWidget(new GpsToggle());
  layout->addWidget(new UiTpmsToggle());
  layout->addWidget(horizontal_line());
  const char* addfunc = "cp -f /data/openpilot/installer/fonts/driver_monitor.py /data/openpilot/selfdrive/monitoring";
  layout->addWidget(new ButtonControl("ADD Func", "Run", "", [=]() {
                                        if (ConfirmationDialog::confirm("Process?")) {
                                          std::system(addfunc);
                                        }
                                      }));
  const char* realdata_clear = "rm -rf /sdcard/realdata/*";
  layout->addWidget(new ButtonControl("Realdata Clear", "Run", "", [=]() {
                                        if (ConfirmationDialog::confirm("Process?")) {
                                          std::system(realdata_clear);
                                        }
                                      }));
  layout->addStretch(1);

  QWidget *w = new QWidget;
  w->setLayout(layout);
  return w;
}

void SettingsWindow::showEvent(QShowEvent *event) {
  if (layout()) {
    panel_widget->setCurrentIndex(0);
    nav_btns->buttons()[0]->setChecked(true);
    return;
  }
}

SettingsWindow::SettingsWindow(QWidget *parent) : QFrame(parent) {
  // setup two main layouts
  QVBoxLayout *sidebar_layout = new QVBoxLayout();
  sidebar_layout->setMargin(0);
  panel_widget = new QStackedWidget();
  panel_widget->setStyleSheet(R"(
    border-radius: 30px;
    background-color: #292929;
  )");

  // close button
  QPushButton *close_btn = new QPushButton("◀");
  close_btn->setStyleSheet(R"(
    font-size: 90px;
    font-weight: bold;
    border 1px grey solid;
    border-radius: 100px;
    background-color: #292929;
  )");
  close_btn->setFixedSize(200, 200);
  sidebar_layout->addSpacing(45);
  sidebar_layout->addWidget(close_btn, 0, Qt::AlignCenter);
  QObject::connect(close_btn, &QPushButton::released, this, &SettingsWindow::closeSettings);

  // setup panels
  DevicePanel *device = new DevicePanel(this);
  QObject::connect(device, &DevicePanel::reviewTrainingGuide, this, &SettingsWindow::reviewTrainingGuide);
  QObject::connect(device, &DevicePanel::showDriverView, this, &SettingsWindow::showDriverView);

  QPair<QString, QWidget *> panels[] = {
    {"Device", device},
    {"Network", network_panel(this)},
    {"Toggles", new TogglesPanel(this)},
    {"Software", new SoftwarePanel()},
  };

  sidebar_layout->addSpacing(45);
  nav_btns = new QButtonGroup();
  for (auto &[name, panel] : panels) {
    QPushButton *btn = new QPushButton(name);
    btn->setCheckable(true);
    btn->setChecked(nav_btns->buttons().size() == 0);
    btn->setStyleSheet(R"(
      QPushButton {
        color: grey;
        border: none;
        background: none;
        font-size: 65px;
        font-weight: 500;
        padding-top: 35px;
        padding-bottom: 35px;
      }
      QPushButton:checked {
        color: white;
      }
    )");

    nav_btns->addButton(btn);
    sidebar_layout->addWidget(btn, 0, Qt::AlignRight);

    panel->setContentsMargins(50, 25, 50, 25);

    ScrollView *panel_frame = new ScrollView(panel, this);
    panel_widget->addWidget(panel_frame);

    QObject::connect(btn, &QPushButton::released, [=, w = panel_frame]() {
      panel_widget->setCurrentWidget(w);
    });
  }
  sidebar_layout->setContentsMargins(50, 50, 100, 50);

  // main settings layout, sidebar + main panel
  QHBoxLayout *settings_layout = new QHBoxLayout();

  sidebar_widget = new QWidget;
  sidebar_widget->setLayout(sidebar_layout);
  sidebar_widget->setFixedWidth(500);
  settings_layout->addWidget(sidebar_widget);
  settings_layout->addWidget(panel_widget);

  setLayout(settings_layout);
  setStyleSheet(R"(
    * {
      color: white;
      font-size: 50px;
    }
    SettingsWindow {
      background-color: black;
    }
  )");
}

void SettingsWindow::hideEvent(QHideEvent *event){
#ifdef QCOM
  HardwareEon::close_activities();
#endif

  // TODO: this should be handled by the Dialog classes
  QList<QWidget*> children = findChildren<QWidget *>();
  for(auto &w : children){
    if(w->metaObject()->superClass()->className() == QString("QDialog")){
      w->close();
    }
  }
}
