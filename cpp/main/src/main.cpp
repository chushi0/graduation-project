#include "mainwindow.h"

#include "rpc/base.h"
#include <QApplication>
#include <QLocale>
#include <QTranslator>

#pragma comment(linker, "/subsystem:\"windows\" /entry:\"mainCRTStartup\"")
int main(int argc, char *argv[]) {
	QApplication a(argc, argv);

	rpc::Init();

	QTranslator translator;
	const QStringList uiLanguages = QLocale::system().uiLanguages();
	for (const QString &locale : uiLanguages) {
		const QString baseName = "main_" + QLocale(locale).name();
		if (translator.load(":/i18n/" + baseName)) {
			a.installTranslator(&translator);
			break;
		}
	}

	MainWindow w;
	w.show();
	w.activateWindow();

	return a.exec();
}
