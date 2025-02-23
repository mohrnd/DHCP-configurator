#ifndef WIDGET_H
#define WIDGET_H
#include "sshcmdexecfiletransfer.h"
#include <QWidget>

QT_BEGIN_NAMESPACE
namespace Ui {
class Widget;
}
QT_END_NAMESPACE

class Widget : public QWidget
{
    Q_OBJECT

public:
    Widget(QWidget *parent = nullptr);
    ~Widget();
    void InsertTextIntoTextEdit(std::string text, QDialog* dialog);


private slots:
    void on_enableRadvdCheckbox_checkStateChanged(const Qt::CheckState &arg1);

    void on_enableIPv6checkbox_checkStateChanged(const Qt::CheckState &arg1);

    void on_AddDomainCheckbox_checkStateChanged(const Qt::CheckState &arg1);

    void on_TestButton_clicked();

    bool test_ssh_connection(SSHCmdExecFileTransfer& ssh, const char* host, const char* user, const char* password);

    void on_ConfigureButton_clicked();

private:
    Ui::Widget *ui;
};
#endif // WIDGET_H
