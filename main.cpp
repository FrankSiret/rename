#include "rename.h"
#include <QApplication>
#include <QStyleFactory>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
//    QApplication::setStyle(QStyleFactory::create("Fusion"));

//    QString arg0 = a.arguments().value(0, "");
    QString arg1 = a.arguments().value(1, "");

//    QMessageBox *msg = new QMessageBox();
//    msg->setWindowTitle("Arguments");
//    msg->setText(QString::number(argc) + ":\n" + arg0 + "/\n" + arg1 + "/");
//    msg->exec();

    a.setApplicationName("Rename Files");
    a.setApplicationDisplayName("Rename Files");
    a.setOrganizationName("Mirage's Company");
    a.setOrganizationDomain("rename.franksiret.project.qt.com.cu");

    Rename w(nullptr, arg1);
    w.show();

    return a.exec();
}
