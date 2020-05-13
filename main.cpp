#include <QtWidgets/QApplication>
#include <QtCore/QCoreApplication>
#include <QString>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QDate>
#include <QImage>
#include <QVector2D>
#include <QColor>
#include <QPainter>
#include <QMap>
#include <QFont>
#include <QCommandLineParser>
#include <QDebug>
#include <iostream>

auto idToColor(std::uint64_t const id) noexcept -> QColor {
	constexpr int d = 256;

	auto const c = static_cast<int64_t>(std::pow(10, 18)) / (d*d*d)+1;
	auto const n = (id) / c;
	auto const r = n / (d * d);
	auto const g = (n - (r * (d * d))) / d;
	auto const b = n - (r * (d * d)) - (g * d);

	return QColor{ (int)r, (int)g, (int)b };
}

int main(int argc, char* argv[])
{
	QGuiApplication a(argc, argv);
	QGuiApplication::setApplicationName("Dc-Plotter");
	QGuiApplication::setApplicationVersion("0.5");

	QCommandLineParser parser;
	parser.setApplicationDescription("Gives a png, with the plotted data of the given json-files. Want more help? TOO BAD!");
	parser.addHelpOption();
	parser.addVersionOption();

	QCommandLineOption showProgressOption("t", QCoreApplication::translate("main", "Show time total (If false, shows time per Day)."));
	parser.addOption(showProgressOption);

	QCommandLineOption startingDateOption("s", QCoreApplication::translate("main", "Starting date in format yyyy-MM-dd."), "a");
	parser.addOption(startingDateOption);

	QCommandLineOption imgWidthOption("m", QCoreApplication::translate("main", "Image width."), "b");
	parser.addOption(imgWidthOption);

	QCommandLineOption imgHeightOption("n", QCoreApplication::translate("main", "Image height."), "c");
	parser.addOption(imgHeightOption);

	QCommandLineOption xLabelsOption("x", QCoreApplication::translate("main", "Number of date labels."), "d");
	parser.addOption(xLabelsOption);

	QCommandLineOption yLabelsOption("y", QCoreApplication::translate("main", "Number of time labels."), "e");
	parser.addOption(yLabelsOption);

	parser.addPositionalArgument("dirPath", QGuiApplication::translate("main", "Source dir."));
	parser.addPositionalArgument("outDir", QGuiApplication::translate("main", "Output dir."));

	parser.process(a);
	const QStringList args = parser.positionalArguments();
	
	bool showProgress = parser.isSet(showProgressOption);
	QDate startingDate = QDate::fromString(parser.value(startingDateOption), "yyyy-MM-dd");
	int imgWidth = parser.value(imgWidthOption).toInt();
	int imgHeight = parser.value(imgHeightOption).toInt();
	int xLabels = parser.value(xLabelsOption).toInt();
	int yLabels = parser.value(yLabelsOption).toInt();
	QString dirPath = args[0] + "/";
	QString outPath = args[1];

	srand(time(NULL));
	

	QImage outputImg = QImage(imgWidth, imgHeight, QImage::Format_RGB32);
	QPainter thisPainter(&outputImg);
	thisPainter.fillRect(0, 0, outputImg.width(), outputImg.height(), QBrush(Qt::white));

	QVector2D drawingPos(0.1, 0.1);
	QVector2D drawingSize(0.8, 0.8);
	thisPainter.setPen(Qt::black);
	thisPainter.drawRect(
		drawingPos[0] * outputImg.width(),
		drawingPos[1] * outputImg.height(),
		drawingSize[0] * outputImg.width(),
		drawingSize[1] * outputImg.height()
		);

	QDate currentDate = QDate::currentDate();
	if (!startingDate.isValid()) {
		return 1;
	}

	QDate itDate = startingDate;


	QString thisJsonTransSrc = dirPath + "trans.json";
	QFile thisJsonTransFile(thisJsonTransSrc);

	if (!thisJsonTransFile.exists()) {
		return 2;
	}

	QMap<QString, QColor> colorsById;
	QMap<QString, QString> namesById;
	QMap<QString, QList<QVector2D>> pointsById;

	thisJsonTransFile.open(QFile::ReadOnly | QFile::Text);
	QString thisJsonTransString;
	thisJsonTransString = thisJsonTransFile.readAll();
	thisJsonTransFile.close();
	QJsonDocument thisJsonTransDoc = QJsonDocument::fromJson(thisJsonTransString.toUtf8());
	QJsonObject thisJsonTransObject = thisJsonTransDoc.object();

	for (QJsonObject::const_iterator it = thisJsonTransObject.begin(); it != thisJsonTransObject.end(); ++it) {
		colorsById[it.key()] = idToColor(it.key().toULongLong());
		namesById[it.key()] = it.value().toString();
	}

	QJsonObject lastStats;

	while (itDate != currentDate.addDays(1)) {
		QString thisJsonStatSrc = dirPath + "stats_" + itDate.toString("yyyy-MM-dd") + ".json";
		QFile thisStatsJsonFile(thisJsonStatSrc);

		if (thisStatsJsonFile.exists()) {
			thisStatsJsonFile.open(QFile::ReadOnly | QFile::Text);
			QString thisStatsJsonString;
			thisStatsJsonString = thisStatsJsonFile.readAll();
			thisStatsJsonFile.close();

			QJsonDocument thisStatsJsonDoc = QJsonDocument::fromJson(thisStatsJsonString.toUtf8());
			QJsonObject thisStatsJsonObject = thisStatsJsonDoc.object();

			for (QJsonObject::const_iterator it = thisStatsJsonObject.begin(); it != thisStatsJsonObject.end(); ++it) {
				float dDays = startingDate.daysTo(itDate);

				float dTime = it.value().toInt();

				if (showProgress) {
					dDays /= startingDate.daysTo(currentDate);
					pointsById[it.key()].push_back(QVector2D(dDays, dTime));
				}
				else
				{
					if (dDays > 0) {
						if (lastStats.contains(it.key())) {
							dTime -= lastStats[it.key()].toInt();
						}

						dDays -= 1;
						dDays /= startingDate.daysTo(currentDate) - 1;

						pointsById[it.key()].push_back(QVector2D(dDays, dTime));
					}
				}
			}

			lastStats = thisStatsJsonObject;
		}

		itDate = itDate.addDays(1);
	}

	int maxY;
	if (showProgress) {
		maxY = 0;
		for (QMap<QString, QList<QVector2D>>::iterator it = pointsById.begin(); it != pointsById.end(); ++it) {
			QList<QVector2D> thisPointsList = it.value();
			for (int i = 1; i < thisPointsList.size(); i++) {
				if (thisPointsList[i][1] > maxY) {
					maxY = thisPointsList[i][1];
				}
			}
		}
		maxY *= 1.2;
	}
	else {
		maxY = 24 * 60 * 60;
	}

	for (QMap<QString, QList<QVector2D>>::iterator it = pointsById.begin(); it != pointsById.end(); ++it) {
		QList<QVector2D> thisPointsList = it.value();
		for (int i = 0; i < thisPointsList.size(); i++) {
			pointsById[it.key()][i][1] /= maxY;
		}
	}

	QFont serifFont("Times", 10, QFont::Bold);
	thisPainter.setFont(serifFont);

	for (QMap<QString, QList<QVector2D>>::iterator it = pointsById.begin(); it != pointsById.end(); ++it) {
		QList<QVector2D> thisPointsList = it.value();

		thisPainter.setPen(colorsById[it.key()]);

		QVector2D lastPoint = thisPointsList[0];
		lastPoint[0] = drawingPos[0] * outputImg.width() + lastPoint[0] * drawingSize[0] * outputImg.width();
		lastPoint[1] = (drawingPos[1] + drawingSize[1]) * outputImg.height() - lastPoint[1] * drawingSize[1] * outputImg.height();

		thisPainter.drawText(lastPoint.toPoint(), namesById[it.key()]);

		for (int i = 1; i < thisPointsList.size(); i++) {
			QVector2D thisPoint = thisPointsList[i];
			thisPoint[0] = drawingPos[0] * outputImg.width() + thisPoint[0] * drawingSize[0] * outputImg.width();
			thisPoint[1] = (drawingPos[1] + drawingSize[1]) * outputImg.height() - thisPoint[1] * drawingSize[1] * outputImg.height();

			thisPainter.drawLine(
				lastPoint[0],
				lastPoint[1],
				thisPoint[0],
				thisPoint[1]
			);
			lastPoint = thisPoint;
		}
	}

	if (xLabels > startingDate.daysTo(currentDate) - !showProgress) {
		xLabels = startingDate.daysTo(currentDate) - !showProgress;
	}

	thisPainter.setPen(Qt::black);

	QFontMetrics fm(serifFont);

	for (int i = 0; i <= xLabels ; i++) {
		QString thisLabelString = startingDate.addDays(round((startingDate.daysTo(currentDate) - !showProgress) * i / xLabels) + !showProgress).toString();
		QVector2D thisLabelPos((drawingPos[0] + drawingSize[0] * (float)i / xLabels) * outputImg.width() - fm.width(thisLabelString) / 2, (drawingPos[1] + drawingSize[1])* outputImg.height() + 1.5 * fm.height());
		thisPainter.drawText(thisLabelPos.toPoint(), thisLabelString);
	}

	for (int i = 0; i <= yLabels; i++) {
		QString thisLabelString = QString::number(round(maxY * (float)i / yLabels / 3600)) + "h";
		QVector2D thisLabelPos(drawingPos[0] * outputImg.width() - fm.width(thisLabelString), (drawingPos[1] + drawingSize[1] - drawingSize[1] * (float)i / yLabels) * outputImg.height() + 0.5 * fm.height());
		thisPainter.drawText(thisLabelPos.toPoint(), thisLabelString);
	}

	QString thisLabelString;
	if (showProgress) {
		thisLabelString = "Time total";
	}
	else {
		thisLabelString = "Time per Day";
	}

	QFont font2("Times", 20, QFont::Bold);
	thisPainter.setFont(font2);
	QFontMetrics fm2(font2);

	thisPainter.drawText(QPoint(0.5 * outputImg.width() - fm2.width(thisLabelString) / 2, 0.05 * outputImg.height()), thisLabelString);
	

	outputImg.save(outPath);

	return a.exec();
}