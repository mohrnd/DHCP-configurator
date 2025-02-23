#include "widget.h"
#include "./ui_widget.h"
#include <vector>
#include "sshcmdexecfiletransfer.h"
#include <iostream>
#include <sstream>
#include <QMessageBox>
#include <QDialog>
#include <QVBoxLayout>
#include <QTextEdit>
#include <QPushButton>
#include <QDir>
#include <QFile>
#include <QTextStream>


Widget::Widget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::Widget)
{
    ui->setupUi(this);
}

Widget::~Widget()
{
    delete ui;
}

void Widget::on_enableRadvdCheckbox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        // Enable forms/widgets when checkbox is checked
        ui->RadvdIpv6NetworkForm->setEnabled(true);
        ui->NICComboboxRadvd->setEnabled(true);

        // Add more widgets as needed
    } else {
        // Disable forms/widgets when checkbox is unchecked
        ui->RadvdIpv6NetworkForm->setEnabled(false);
        ui->NICComboboxRadvd->setEnabled(false);
    }
}


void Widget::on_enableIPv6checkbox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        // Enable forms/widgets when checkbox is checked
        ui->IPv6NetworkForm->setEnabled(true);
        ui->IPv6MaskForm->setEnabled(true);
        ui->IPV6ServerIPform->setEnabled(true);
        ui->DHCPv6Range1->setEnabled(true);
        ui->DHCPv6Range2->setEnabled(true);
    } else {
        // Disable forms/widgets when checkbox is unchecked
        ui->IPv6NetworkForm->setEnabled(false);
        ui->IPv6MaskForm->setEnabled(false);
        ui->IPV6ServerIPform->setEnabled(false);
        ui->DHCPv6Range1->setEnabled(false);
        ui->DHCPv6Range2->setEnabled(false);
    }
}


void Widget::on_AddDomainCheckbox_checkStateChanged(const Qt::CheckState &arg1)
{
    if (arg1 == Qt::Checked) {
        // Enable forms/widgets when checkbox is checked
        ui->DomainForm->setEnabled(true);
        ui->IPV4dnsServerForm->setEnabled(true);
        ui->IPV6dnsServerform->setEnabled(true);
    } else {
        // Disable forms/widgets when checkbox is unchecked
        ui->DomainForm->setEnabled(false);
        ui->IPV4dnsServerForm->setEnabled(false);
        ui->IPV6dnsServerform->setEnabled(false);
    }
}



void Widget::on_TestButton_clicked()
{
    QString IPAddress = ui->IPAddress->text().trimmed();
    QString RootUser = ui->RootUser->text().trimmed();
    QString Password = ui->Password->text().trimmed();

    // **Check if any required fields are empty**
    if (IPAddress.isEmpty() || RootUser.isEmpty() || Password.isEmpty()) {
        QMessageBox::warning(this, "Input Error", "Please fill in all fields before proceeding.");
        return;  // Stop execution
    }

    QByteArray byteArray1 = IPAddress.toUtf8();
    QByteArray byteArray2 = RootUser.toUtf8();
    QByteArray byteArray3 = Password.toUtf8();

    const char* IPAddress_char = byteArray1.constData();
    const char* RootUser_char = byteArray2.constData();
    const char* Password_char = byteArray3.constData();

    SSHCmdExecFileTransfer ssh;

    // **Test SSH connection**
    bool connState = test_ssh_connection(ssh, IPAddress_char, RootUser_char, Password_char);
    ui->ConnectivityForm->setText(connState ? "Yes" : "No");

    if (!connState) {
        QMessageBox::critical(this, "SSH Connection Failed", "Failed to establish an SSH connection.");
        return;
    }

    // **Check Internet connectivity**
    const char* pingCommand = "ping -c 2 8.8.8.8 > /dev/null 2>&1 && echo Yes || echo No";
    SSHCmdExecFileTransfer::CommandResult ping_result = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, pingCommand);
    ui->InternetForm->setText(QString::fromStdString(ping_result.output));

    // **Retrieve OS version**
    const char* osCommand = "grep PRETTY_NAME /etc/os-release | cut -d '=' -f 2 | tr -d '\"'";
    SSHCmdExecFileTransfer::CommandResult os_result = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, osCommand);
    ui->DetectedOSForm->setText(QString::fromStdString(os_result.output));

    // **Retrieve network interfaces**
    const char* nicCommand = "ls /sys/class/net";
    SSHCmdExecFileTransfer::CommandResult nic_result = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, nicCommand);

    std::vector<std::string> nic_list;
    std::istringstream iss(nic_result.output);
    std::string nic;
    while (std::getline(iss, nic)) {
        nic_list.push_back(nic);
    }

    ui->NICComboboxIPV4->clear();
    ui->NICComboboxIPV6->clear();
    ui->NICComboboxRadvd->clear();

    for (const auto& nic : nic_list) {
        ui->NICComboboxIPV4->addItem(QString::fromStdString(nic));
        ui->NICComboboxIPV6->addItem(QString::fromStdString(nic));
        ui->NICComboboxRadvd->addItem(QString::fromStdString(nic));
    }
}




// Function to test SSH connection
bool Widget::test_ssh_connection(SSHCmdExecFileTransfer& ssh, const char* host, const char* user, const char* password) {
    std::cout << "Testing SSH connection..." << std::endl;
    auto conn_result = ssh.test_connection(host, user, password);
    if (!conn_result.success) {
        std::cerr << "Connection test failed: " << conn_result.error_message << std::endl;
        std::cerr << "Error code: " << conn_result.error_code << std::endl;
        return false;
    } else {
        std::cout << "Connection test successful!" << std::endl;
        return true;
    }
}

void Widget::on_ConfigureButton_clicked()
{
    QDialog *dialog = new QDialog(this);
    dialog->setWindowTitle("Configuration Details");
    dialog->resize(600, 400);
    QVBoxLayout *layout = new QVBoxLayout(dialog);
    QTextEdit *textEdit = new QTextEdit(dialog);
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);
    QPushButton *Close = new QPushButton("Close", dialog);
    layout->addWidget(Close);
    Close->setEnabled(false);
    QString OS = ui->DetectedOSForm->text().trimmed();
    textEdit->append("<b>Configuration for: </b><span style='color: green;'>" + OS + "</span>");
    QApplication::processEvents();

    connect(Close, &QPushButton::clicked, dialog, &QDialog::close);
    dialog->show();
    QApplication::processEvents();

    QString IPAddress = ui->IPAddress->text().trimmed();
    QString RootUser = ui->RootUser->text().trimmed();
    QString Password = ui->Password->text().trimmed();
    QByteArray byteArray1 = IPAddress.toUtf8();
    QByteArray byteArray2 = RootUser.toUtf8();
    QByteArray byteArray3 = Password.toUtf8();

    const char* IPAddress_char = byteArray1.constData();
    const char* RootUser_char = byteArray2.constData();
    const char* Password_char = byteArray3.constData();

    SSHCmdExecFileTransfer ssh;
    const char* osCommand1 = "echo made by mohrnd";
    QApplication::processEvents();
    SSHCmdExecFileTransfer::CommandResult os_result = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, osCommand1);
    textEdit->append(QString::fromStdString(os_result.output));


    QApplication::processEvents();
    // Check if DHCP and Radvd are installed
    if (!ui->NoInternetNeeded->isChecked() && ui->ConnectivityForm->text() == "Yes") {
        textEdit->append("<b>Installing DHCP</b>");
        const char* installDHCP = "apt-get -y update && apt-get -y upgrade && apt-get -y install isc-dhcp-common isc-dhcp-server";
        textEdit->append("<span style='color: orange;'>Executing: </span>" + QString(installDHCP));
        QApplication::processEvents();
        SSHCmdExecFileTransfer::CommandResult dhcp_result = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, installDHCP);
        textEdit->append(QString::fromStdString(dhcp_result.output));
        if (dhcp_result.success) {
            textEdit->append("<span style='color: green;'>DHCP server installed successfully</span>");
        } else {
            textEdit->append("<span style='color: red;'>Error installing DHCP server</span>");
        }
        QApplication::processEvents();
    }

    // Check if Radvd should be installed
    if (ui->enableRadvdCheckbox->isChecked() && !ui->NoInternetNeeded->isChecked() && ui->ConnectivityForm->text() == "Yes") {
        textEdit->append("<b>Installing Radvd</b>");
        const char* installRadvd = "apt-get -y install radvd";
        textEdit->append("<span style='color: orange;'>Executing: </span>" + QString(installRadvd));
        QApplication::processEvents();
        SSHCmdExecFileTransfer::CommandResult radvd_result = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, installRadvd);
        textEdit->append(QString::fromStdString(radvd_result.output));
        if (radvd_result.success) {
            textEdit->append("<span style='color: green;'>Radvd installed successfully</span>");
        } else {
            textEdit->append("<span style='color: red;'>Error installing Radvd</span>");
        }
        QApplication::processEvents();
    }
    QApplication::processEvents();
    QString ipv4interface = ui->NICComboboxIPV4->currentText().trimmed();
    QString ipv4Address = ui->ServerIPv4Form_2->text().trimmed();
    QString netmask = ui->NetoworkIPv4MaskForm->text().trimmed();
    QString broadcast = ui->BroadcastAddressForm->text().trimmed();
    QString gateway = ui->DefaultGwForm->text().trimmed();

    QString ipv6Address, ipv6Netmask, ipv6interface;
    bool ipv6Enabled = ui->enableIPv6checkbox->isChecked();
    if (ipv6Enabled) {
        ipv6Address = ui->IPV6ServerIPform->text().trimmed();
        ipv6Netmask = ui->IPv6MaskForm->text().trimmed();
        ipv6interface = ui->NICComboboxIPV6->currentText().trimmed();
    }

    textEdit->append("<b>Networking Configuration</b>");
    QApplication::processEvents();
    // Backup existing interfaces file
    const char* backupCommand = "mv /etc/network/interfaces /etc/network/interfaces.backup";
    SSHCmdExecFileTransfer::CommandResult backupResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, backupCommand);
    if (backupResult.success) {
        textEdit->append("<span style='color: green;'>Backup of network configuration successful</span>");
    } else {
        textEdit->append("<span style='color: red;'>Failed to backup network configuration</span>");
        return;
    }
    QApplication::processEvents();
    // Retrieve the existing interfaces file
    const char* catCommand = "cat /etc/network/interfaces.backup";
    SSHCmdExecFileTransfer::CommandResult oldConfig = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, catCommand);
    if (oldConfig.success) {
        textEdit->append("<span style='color: green;'>Retrieved existing network configuration</span>");
    } else {
        textEdit->append("<span style='color: red;'>Failed to retrieve existing network configuration</span>");
        return;
    }
    QApplication::processEvents();
    // Modify the content
    QString newConfig;
    QStringList lines = QString::fromStdString(oldConfig.output).split("\n");
    bool autoLoModified = false;

    for (QString line : lines) {
        if (line.startsWith("auto")) {
            if (!line.contains(ipv4interface)) {
                line += " " + ipv4interface;
            }
            if (ipv6Enabled && !line.contains(ipv6interface)) {
                line += " " + ipv6interface;
                autoLoModified = true;
            }
        }
        newConfig += line + "\n";
    }

    // Add the IPv4 configuration
    newConfig += "\niface " + ipv4interface + " inet static\n";
    newConfig += "    address " + ipv4Address + "\n";
    newConfig += "    netmask " + netmask + "\n";
    newConfig += "    broadcast " + broadcast + "\n";
    newConfig += "    gateway " + gateway + "\n\n";

    // Add IPv6 if enabled
    if (ipv6Enabled) {
        newConfig += "iface " + ipv6interface + " inet6 static\n";
        newConfig += "    address " + ipv6Address + "\n";
        newConfig += "    netmask " + ipv6Netmask + "\n\n";
    }

    textEdit->append("<span style='color: green;'>Network configuration modified successfully</span>");
    QApplication::processEvents();
    // Save config to temp file
    QString localFilePath = QDir::temp().filePath("interfaces");
    QFile file(localFilePath);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << newConfig;
        file.close();
        textEdit->append("<span style='color: green;'>Temporary network config file created</span>");
    } else {
        textEdit->append("<span style='color: red;'>Failed to create temporary config file</span>");
        return;
    }
    QApplication::processEvents();
    // Upload the modified file back to the server
    SSHCmdExecFileTransfer::FileTransferResult result = ssh.upload_file(
        IPAddress_char, RootUser_char, Password_char,
        localFilePath.toUtf8().constData(), "/etc/network/interfaces"
        );

    if (result.success) {
        textEdit->append("<span style='color: green;'>Network configuration updated successfully</span>");
    } else {
        textEdit->append("<span style='color: red;'>Failed to upload network configuration file</span>");
    }
    QApplication::processEvents();
    const char* RestartNetworking = "systemctl restart networking";
    SSHCmdExecFileTransfer::CommandResult restartResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, RestartNetworking);
    if (restartResult.success) {
        textEdit->append("<span style='color: green;'>Network services restarted successfully</span>");
    } else {
        textEdit->append("<span style='color: red;'>Failed to restart network services</span>");
        return;
    }
    QApplication::processEvents();

    //////////////RADVD config

    QString radvdInterface = ui->NICComboboxRadvd->currentText().trimmed();
    QString radvdIpv6Network = ui->RadvdIpv6NetworkForm->text().trimmed();
    bool radvdEnabled = ui->enableRadvdCheckbox->isChecked();

    if (radvdEnabled) {
        textEdit->append("<b>Radvd Configuration</b>");
        // Check if the radvd.conf file already exists
        const char* checkFileCommand = "cd /etc/ ;  ls | grep radvd.conf | echo $?";
        SSHCmdExecFileTransfer::CommandResult checkFileResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, checkFileCommand);

        // If the file exists, back it up
        std::string res = checkFileResult.output;
        if (checkFileResult.success && res.find("1") != std::string::npos) {
            const char* backupRadvdCommand = "mv /etc/radvd.conf /etc/radvd.conf.backup";
            SSHCmdExecFileTransfer::CommandResult backupRadvdResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, backupRadvdCommand);
            if (backupRadvdResult.success) {
                textEdit->append("<span style='color: green;'>Backup of existing radvd configuration successful</span>");
            } else {
                textEdit->append("<span style='color: red;'>Failed to backup existing radvd configuration</span>");
                return;
            }
        }
        QApplication::processEvents();

        // Create the radvd config string
        QString radvdConfig;
        radvdConfig += "interface " + radvdInterface + " {\n";
        radvdConfig += "   AdvSendAdvert on;\n";
        radvdConfig += "   prefix " + radvdIpv6Network + " {\n";
        radvdConfig += "   };\n";
        radvdConfig += "};\n";

        textEdit->append("<span style='color: green;'>Radvd configuration generated successfully</span>");
        QApplication::processEvents();
        // Save radvd configuration to a temp file
        QString radvdLocalFilePath = QDir::temp().filePath("radvd.conf");
        QFile radvdFile(radvdLocalFilePath);
        if (radvdFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&radvdFile);
            out << radvdConfig;
            radvdFile.close();
            textEdit->append("<span style='color: green;'>Temporary radvd config file created</span>");
        } else {
            textEdit->append("<span style='color: red;'>Failed to create temporary radvd config file</span>");
            return;
        }
        QApplication::processEvents();
        // Upload the radvd config file to the server
        SSHCmdExecFileTransfer::FileTransferResult radvdResult = ssh.upload_file(
            IPAddress_char, RootUser_char, Password_char,
            radvdLocalFilePath.toUtf8().constData(), "/etc/radvd.conf"
            );

        if (radvdResult.success) {
            textEdit->append("<span style='color: green;'>Radvd configuration updated successfully</span>");
        } else {
            textEdit->append("<span style='color: red;'>Failed to upload radvd configuration file</span>");
        }

        QApplication::processEvents();
        const char* RestartRadvd = "systemctl restart radvd";
        SSHCmdExecFileTransfer::CommandResult restartResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, RestartRadvd);

        if (restartResult.success) {
            textEdit->append("<span style='color: green;'>Network services restarted successfully.</span>");
        } else {
            textEdit->append("<span style='color: red;'>Failed to restart network services. Please check the system logs for more details.</span>");
            return;
        }
        QApplication::processEvents();

////////////////////DHCP Config
        textEdit->append("<b>DHCP Configuration</b>");
        QApplication::processEvents();

        const char* backupDhcpCommand = "mv /etc/default/isc-dhcp-server /etc/default/isc-dhcp-server.backup";
        SSHCmdExecFileTransfer::CommandResult backupDhcpResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, backupDhcpCommand);
        if (backupDhcpResult.success) {
            textEdit->append("<span style='color: green;'>Backup of isc-dhcp-server configuration successful</span>");
        } else {
            textEdit->append("<span style='color: red;'>Failed to backup isc-dhcp-server configuration</span>");
            return;
        }
        QApplication::processEvents();
        // Create a new isc-dhcp-server configuration file
        QString newDhcpConfig;
        newDhcpConfig += "INTERFACESv4=\"" + ui->NICComboboxIPV4->currentText().trimmed() + "\"\n";
        newDhcpConfig += "INTERFACESv6=\"" + ui->NICComboboxIPV6->currentText().trimmed() + "\"\n";

        // Save new isc-dhcp-server config to a temp file
        QString dhcpLocalFilePath = QDir::temp().filePath("isc-dhcp-server");
        QFile dhcpFile(dhcpLocalFilePath);
        if (dhcpFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&dhcpFile);
            out << newDhcpConfig;
            dhcpFile.close();
            textEdit->append("<span style='color: green;'>Temporary isc-dhcp-server config file created</span>");
        } else {
            textEdit->append("<span style='color: red;'>Failed to create temporary isc-dhcp-server config file</span>");
            return;
        }
        QApplication::processEvents();
        // Upload the new isc-dhcp-server file to the server
        SSHCmdExecFileTransfer::FileTransferResult dhcpResult = ssh.upload_file(
            IPAddress_char, RootUser_char, Password_char,
            dhcpLocalFilePath.toUtf8().constData(), "/etc/default/isc-dhcp-serverconf"
            );

        if (dhcpResult.success) {
            textEdit->append("<span style='color: green;'>isc-dhcp-server configuration updated successfully</span>");
        } else {
            textEdit->append("<span style='color: red;'>Failed to upload isc-dhcp-server configuration file</span>");
        }

        QApplication::processEvents();
        // First backup dhcpd.conf
        const char* backupDhcpdCommand = "mv /etc/dhcp/dhcpd.conf /etc/dhcp/dhcpd.conf.backup";
        SSHCmdExecFileTransfer::CommandResult backupDhcpdResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, backupDhcpdCommand);
        if (backupDhcpdResult.success) {
            textEdit->append("<span style='color: green;'>Backup of dhcpd.conf configuration successful</span>");
        } else {
            textEdit->append("<span style='color: red;'>Failed to backup dhcpd.conf configuration</span>");
            return;
        }

        // Retrieve the existing dhcpd.conf content
        const char* catDhcpCommand = "cat /etc/dhcp/dhcpd.conf.backup";
        SSHCmdExecFileTransfer::CommandResult dhcpConfResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, catDhcpCommand);

        if (dhcpConfResult.success) {
            QString modifiedDhcpConf;
            QStringList lines = QString::fromStdString(dhcpConfResult.output).split("\n");
            bool domainNameFound = false;
            bool dnsServersFound = false;

            // Process each line
            for (const QString& line : lines) {
                QString trimmedLine = line.trimmed();
                QString modifiedLine = line;

                // Skip empty lines
                if (trimmedLine.isEmpty()) {
                    modifiedDhcpConf += modifiedLine + "\n";
                    continue;
                }

                // Check if line is commented
                if (trimmedLine.startsWith('#')) {
                    modifiedDhcpConf += modifiedLine + "\n";
                    continue;
                }

                if (line.contains("option domain-name ") && !trimmedLine.startsWith('#')) {
                    modifiedLine = "option domain-name \"" + ui->DomainForm->text() + "\";";
                    domainNameFound = true;
                }
                else if (line.contains("option domain-name-servers") && !trimmedLine.startsWith('#')) {
                    modifiedLine = "option domain-name-servers " + ui->IPV4dnsServerForm->text() + ";";
                    dnsServersFound = true;
                }
                modifiedDhcpConf += modifiedLine + "\n";
            }

            if (!domainNameFound) {
                modifiedDhcpConf += "option domain-name \"" + ui->DomainForm->text() + "\";\n";
                textEdit->append("<span style='color: orange;'>Added missing domain-name configuration</span>");
            }
            if (!dnsServersFound) {
                modifiedDhcpConf += "option domain-name-servers " + ui->IPV4dnsServerForm->text() + ";\n";
                textEdit->append("<span style='color: orange;'>Added missing DNS servers configuration</span>");
            }
            QApplication::processEvents();
            // Add subnet block at the end
            modifiedDhcpConf += "\nsubnet " + ui->NetworkIPv4Form->text() + " netmask " + ui->NetoworkIPv4MaskForm->text() + " {\n";
            modifiedDhcpConf += "    range " + ui->IPV4range1->text() + " " + ui->IPV4Range2->text() + ";\n";
            // modifiedDhcpConf += "    option subnet-mask " + ui->NetoworkIPv4MaskForm->text() + ";\n";
            modifiedDhcpConf += "    option broadcast-address " + ui->BroadcastAddressForm->text() + ";\n";
            modifiedDhcpConf += "    option routers " + ui->DefaultGwForm->text() + ";\n";
            modifiedDhcpConf += "}\n";

            // Save modified dhcpd.conf to a temp file
            QString dhcpConfLocalFilePath = QDir::temp().filePath("dhcpd.conf");
            QFile dhcpConfFile(dhcpConfLocalFilePath);
            if (dhcpConfFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&dhcpConfFile);
                out << modifiedDhcpConf;
                dhcpConfFile.close();
                textEdit->append("<span style='color: green;'>Temporary dhcpd.conf file created</span>");
            } else {
                textEdit->append("<span style='color: red;'>Failed to create temporary dhcpd.conf file</span>");
                return;
            }
            QApplication::processEvents();
            // Upload the modified dhcpd.conf
            SSHCmdExecFileTransfer::FileTransferResult dhcpConfUploadResult = ssh.upload_file(
                IPAddress_char, RootUser_char, Password_char,
                dhcpConfLocalFilePath.toUtf8().constData(), "/etc/dhcp/dhcpd.conf"
                );

            if (dhcpConfUploadResult.success) {
                textEdit->append("<span style='color: green;'>dhcpd.conf configuration updated successfully</span>");
            } else {
                textEdit->append("<span style='color: red;'>Failed to upload dhcpd.conf configuration file</span>");
            }
            QApplication::processEvents();
        }

        // Similarly for dhcpd6.conf
        // First backup dhcpd6.conf
        const char* backupDhcpd6Command = "mv /etc/dhcp/dhcpd6.conf /etc/dhcp/dhcpd6.conf.backup";
        SSHCmdExecFileTransfer::CommandResult backupDhcpd6Result = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, backupDhcpd6Command);
        if (backupDhcpd6Result.success) {
            textEdit->append("<span style='color: green;'>Backup of dhcpd6.conf configuration successful</span>");
        } else {
            textEdit->append("<span style='color: red;'>Failed to backup dhcpd6.conf configuration</span>");
            return;
        }
        QApplication::processEvents();
        // Retrieve the existing dhcpd6.conf content
        const char* catDhcp6Command = "cat /etc/dhcp/dhcpd6.conf.backup";
        SSHCmdExecFileTransfer::CommandResult dhcp6ConfResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, catDhcp6Command);

        if (dhcp6ConfResult.success) {
            QString modifiedDhcp6Conf;
            QStringList lines = QString::fromStdString(dhcp6ConfResult.output).split("\n");
            bool nameServersFound = false;
            bool domainSearchFound = false;

            // Process each line
            for (const QString& line : lines) {
                QString trimmedLine = line.trimmed();
                QString modifiedLine = line;

                // Skip empty lines
                if (trimmedLine.isEmpty()) {
                    modifiedDhcp6Conf += modifiedLine + "\n";
                    continue;
                }

                // Check if line is commented
                if (trimmedLine.startsWith('#')) {
                    modifiedDhcp6Conf += modifiedLine + "\n";
                    continue;
                }

                if (line.contains("option dhcp6.name-servers ") && !trimmedLine.startsWith('#')) {
                    modifiedLine = "option dhcp6.name-servers " + ui->IPV6dnsServerform->text() + ";";
                    nameServersFound = true;
                }
                else if (line.contains("option dhcp6.domain-search ") && !trimmedLine.startsWith('#')) {
                    modifiedLine = "option dhcp6.domain-search \"" + ui->DomainForm->text() + "\";";
                    domainSearchFound = true;
                }
                modifiedDhcp6Conf += modifiedLine + "\n";
            }
            QApplication::processEvents();

            if (!nameServersFound) {
                modifiedDhcp6Conf += "option dhcp6.name-servers " + ui->IPV6dnsServerform->text() + ";\n";
                textEdit->append("<span style='color: orange;'>Added missing IPv6 name servers configuration</span>");
            }
            if (!domainSearchFound) {
                modifiedDhcp6Conf += "option dhcp6.domain-search \"" + ui->DomainForm->text() + "\";\n";
                textEdit->append("<span style='color: orange;'>Added missing IPv6 domain search configuration</span>");
            }
            QApplication::processEvents();

            modifiedDhcp6Conf += "\nsubnet6 " + ui->IPv6NetworkForm->text() + "/" + ui->IPv6MaskForm->text() + " {\n";
            modifiedDhcp6Conf += "    range6 " + ui->DHCPv6Range1->text() + " " + ui->DHCPv6Range2->text() + ";\n";
            modifiedDhcp6Conf += "}\n";

            QString dhcp6ConfLocalFilePath = QDir::temp().filePath("dhcpd6.conf");
            QFile dhcp6ConfFile(dhcp6ConfLocalFilePath);
            if (dhcp6ConfFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream out(&dhcp6ConfFile);
                out << modifiedDhcp6Conf;
                dhcp6ConfFile.close();
                textEdit->append("<span style='color: green;'>Temporary dhcpd6.conf file created</span>");
            } else {
                textEdit->append("<span style='color: red;'>Failed to create temporary dhcpd6.conf file</span>");
                return;
            }
            QApplication::processEvents();

            SSHCmdExecFileTransfer::FileTransferResult dhcp6ConfUploadResult = ssh.upload_file(
                IPAddress_char, RootUser_char, Password_char,
                dhcp6ConfLocalFilePath.toUtf8().constData(), "/etc/dhcp/dhcpd6.conf"
                );

            if (dhcp6ConfUploadResult.success) {
                textEdit->append("<span style='color: green;'>dhcpd6.conf configuration updated successfully</span>");
            } else {
                textEdit->append("<span style='color: red;'>Failed to upload dhcpd6.conf configuration file</span>");
            }
            QApplication::processEvents();

        }
        const char* RestartDHCP = "systemctl restart isc-dhcp-server && systemctl enable isc-dhcp-server";
         restartResult = ssh.execute_remote_command(IPAddress_char, RootUser_char, Password_char, RestartDHCP);

        if (restartResult.success) {
            textEdit->append("<span style='color: green;'>DHCP services have been successfully restarted and enabled to start on boot.</span>");
        } else {
            textEdit->append("<span style='color: red;'>DHCP services restart failed. Please check the system logs for further details.</span>");
            textEdit->append("<span style='color: red;'>Error Message: " + QString::fromStdString(restartResult.error_message) + "</span>");
            return;
        }
        QApplication::processEvents();

        textEdit->append("<b><h3>DHCP Server ready!</h3></b>");

        Close->setEnabled(true);
        QApplication::processEvents();

    }
}


