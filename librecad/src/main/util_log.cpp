#include "util_log.h"
#include <QtWidgets>
#include <iostream>

void do_print(const std::string& msg) {
#ifdef Q_OS_WIN
    QString str(msg.c_str());
    std::wcout << (wchar_t*)str.utf16() << L"\n";
#else
    std::cout << msg << "\n";
#endif
}

void show_log(const std::string& msg) {
    do_print(msg);
    QMessageBox msgBox;
    msgBox.setText(msg.c_str());
    msgBox.exec();
}
