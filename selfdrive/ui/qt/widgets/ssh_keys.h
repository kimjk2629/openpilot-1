#pragma once

#include <QPushButton>

#include "selfdrive/hardware/hw.h"
#include "selfdrive/ui/qt/widgets/controls.h"

// SSH enable toggle
class SshToggle : public ToggleControl {
  Q_OBJECT

public:
  SshToggle() : ToggleControl("SSH Enable", "", "../assets/offroad/icon_ssh.png", Hardware::get_ssh_enabled()) {
    QObject::connect(this, &SshToggle::toggleFlipped, [=](bool state) {
      Hardware::set_ssh_enabled(state);
    });
  }
};

// SSH key management widget
class SshControl : public AbstractControl {
  Q_OBJECT

public:
  SshControl();

private:
  Params params;

  QPushButton btn;
  QLabel username_label;

  void refresh();
  void getUserKeys(const QString &username);
};

// prebuilt
class PrebuiltToggle : public ToggleControl {
  Q_OBJECT

public:
  PrebuiltToggle() : ToggleControl("Prebuilt Enable", "", "../assets/offroad/icon_addon.png", Params().getBool("PutPrebuilt")) {
    QObject::connect(this, &PrebuiltToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().put("PutPrebuilt", &value, 1);
    });
  }
};

// DisableShutdownd
class ShutdowndToggle : public ToggleControl {
  Q_OBJECT

public:
  ShutdowndToggle() : ToggleControl("Shutdownd Disable", "", "../assets/offroad/icon_addon.png", Params().getBool("DisableShutdownd")) {
    QObject::connect(this, &ShutdowndToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().put("DisableShutdownd", &value, 1);
    });
  }
};

// DisableLogger
class LoggerToggle : public ToggleControl {
  Q_OBJECT

public:
  LoggerToggle() : ToggleControl("Logger Disable", "", "../assets/offroad/icon_addon.png", Params().getBool("DisableLogger")) {
    QObject::connect(this, &LoggerToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().put("DisableLogger", &value, 1);
    });
  }
};

// DisableGps
class DisableGpsToggle : public ToggleControl {
  Q_OBJECT

public:
  DisableGpsToggle() : ToggleControl("Gps Disable", "", "../assets/offroad/icon_addon.png", Params().getBool("DisableGps")) {
    QObject::connect(this, &DisableGpsToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().put("DisableGps", &value, 1);
    });
  }
};

// Ui Tpms
class UiTpmsToggle : public ToggleControl {
  Q_OBJECT

public:
  UiTpmsToggle() : ToggleControl("Ui Tpms Enable", "", "../assets/offroad/icon_addon.png", Params().getBool("UiTpms")) {
    QObject::connect(this, &UiTpmsToggle::toggleFlipped, [=](int state) {
      char value = state ? '1' : '0';
      Params().put("UiTpms", &value, 1);
    });
  }
};

// LateralControlSelect
class LateralControlSelect : public AbstractControl {
  Q_OBJECT

public:
  LateralControlSelect();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

// MfcSelect
class MfcSelect : public AbstractControl {
  Q_OBJECT

public:
  MfcSelect();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};

// LongControlSelect
class LongControlSelect : public AbstractControl {
  Q_OBJECT

public:
  LongControlSelect();

private:
  QPushButton btnplus;
  QPushButton btnminus;
  QLabel label;

  void refresh();
};
