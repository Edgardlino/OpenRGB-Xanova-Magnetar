#include "XanovaMagnetarPlugin.h"
#include "XanovaMagnetarController.h"

#include <QVBoxLayout>

XanovaMagnetarPlugin::XanovaMagnetarPlugin()
    : resource_manager(nullptr),
      keyboard_controller(nullptr),
      status_label(nullptr)
{
}

XanovaMagnetarPlugin::~XanovaMagnetarPlugin()
{
    Unload();
}

OpenRGBPluginInfo XanovaMagnetarPlugin::GetPluginInfo()
{
    OpenRGBPluginInfo info;
    info.Name        = "Xanova Magnetar RGB";
    info.Description = "Suporte nativo ao teclado Xanova Magnetar RGB ABNT2 (VID 0x1024 / PID 0x2030)";
    info.Version     = "1.0.0";
    info.Commit      = "local";
    info.URL         = "https://gitlab.com/CalcProgrammer1/OpenRGB";
    info.Location    = OPENRGB_PLUGIN_LOCATION_SETTINGS;
    info.Label       = "Xanova Magnetar";
    return info;
}

unsigned int XanovaMagnetarPlugin::GetPluginAPIVersion()
{
    return OPENRGB_PLUGIN_API_VERSION;
}

void XanovaMagnetarPlugin::Load(ResourceManagerInterface* resource_manager_ptr)
{
    resource_manager = resource_manager_ptr;
    TryRegisterKeyboard();
}

QWidget* XanovaMagnetarPlugin::GetWidget()
{
    QWidget* widget = new QWidget();
    QVBoxLayout* layout = new QVBoxLayout(widget);

    status_label = new QLabel(widget);
    UpdateStatusLabel();

    QPushButton* redetect_btn = new QPushButton("Re-detectar Xanova Magnetar", widget);
    connect(redetect_btn, &QPushButton::clicked, this, &XanovaMagnetarPlugin::OnRedetectClicked);

    layout->addWidget(status_label);
    layout->addWidget(redetect_btn);
    layout->addStretch();

    return widget;
}

QMenu* XanovaMagnetarPlugin::GetTrayMenu()
{
    return nullptr;
}

void XanovaMagnetarPlugin::Unload()
{
    if (resource_manager && keyboard_controller)
    {
        resource_manager->UnregisterRGBController(keyboard_controller);
        delete keyboard_controller;
        keyboard_controller = nullptr;
    }
    resource_manager = nullptr;
}

void XanovaMagnetarPlugin::OnRedetectClicked()
{
    if (!keyboard_controller)
    {
        TryRegisterKeyboard();
    }
    UpdateStatusLabel();
}

void XanovaMagnetarPlugin::TryRegisterKeyboard()
{
    if (!resource_manager || keyboard_controller)
    {
        return;
    }

    auto* dev = new XanovaMagnetarController();
    if (dev->Open())
    {
        keyboard_controller = new RGBController_XanovaMagnetar(dev);
        resource_manager->RegisterRGBController(keyboard_controller);
    }
    else
    {
        delete dev;
    }
}

void XanovaMagnetarPlugin::UpdateStatusLabel()
{
    if (!status_label)
    {
        return;
    }
    if (keyboard_controller)
    {
        status_label->setText(QString("Status: Conectado (%1)").arg(QString::fromStdString(keyboard_controller->version)));
    }
    else
    {
        status_label->setText("Status: Teclado Xanova Magnetar (1024:2030) não encontrado.");
    }
}
