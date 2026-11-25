#pragma once

#include <QObject>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include "OpenRGBPluginInterface.h"
#include "RGBController_XanovaMagnetar.h"

class XanovaMagnetarPlugin : public QObject, public OpenRGBPluginInterface
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID OpenRGBPluginInterface_IID)
    Q_INTERFACES(OpenRGBPluginInterface)

public:
    XanovaMagnetarPlugin();
    ~XanovaMagnetarPlugin() override;

    OpenRGBPluginInfo   GetPluginInfo() override;
    unsigned int        GetPluginAPIVersion() override;
    void                Load(ResourceManagerInterface* resource_manager_ptr) override;
    QWidget*            GetWidget() override;
    QMenu*              GetTrayMenu() override;
    void                Unload() override;

private slots:
    void OnRedetectClicked();

private:
    void TryRegisterKeyboard();
    void UpdateStatusLabel();

    ResourceManagerInterface*     resource_manager;
    RGBController_XanovaMagnetar* keyboard_controller;
    QLabel*                       status_label;
};
