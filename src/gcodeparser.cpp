#include "headers/gcodeparser.h"
#include <QFileInfo>
#include <QDebug>

#define min(x, y) ((x<y) ? x : y)

QMap<QString, QString> GCodeParser::parseFile(QString filepath) {
    if (!QFile::exists(filepath)) {
        qWarning() << "File does not exist" << filepath;
        return QMap<QString, QString>();
    }
    QFileInfo fileInfo = QFileInfo(filepath);
    QMap<QString, QString> output;
    if (fileInfo.fileName().toLower().endsWith(".gcode")) {
        qDebug() << "parsing gcode";
        output = parseGCode(readGCode(QFile(filepath)));
    } else if (fileInfo.fileName().toLower().endsWith(".bgcode")) {
         qDebug() << "parsing bgcode";
        output = parseBGCode(readBGCode(QFile(filepath)));
    } else if (fileInfo.fileName().toLower().endsWith(".gcode.3mf")) {
        //handle extraction, then parse gcode
    } else {
        qWarning() << "Invalid filetype" << filepath;
        return QMap<QString, QString>();
    }

    output.insert("filename", QFileInfo(filepath).fileName());
    return output;
}

QMap<QString, QString> GCodeParser::parseGCode(QVector<QString> lines) {
    QMap<QString, QString> properties = QMap<QString, QString>();
    qDebug() << lines.size();
    for (int i = 0; i < min(lines.size(), 6000); i++) {
        QString line = lines[i];
        for (int p = 0; p < GCODE_TARGETS.size(); p++) {
            if (line.startsWith("; " + GCODE_TARGETS[p] + " = ")) {
                QStringList splitLine = line.split("=");
                splitLine.pop_front();
                QString v = splitLine.join("=").trimmed();
                properties.insert(GCODE_TARGETS[p], v);
                break;
            }
        }
        if (line.startsWith("; model printing time:")) { //Bambu abnormality
            QStringList splitLine = line.split(";")[1].split(":");
            splitLine.pop_front();
            QString v = splitLine.join(":").trimmed();
            properties.insert(GCODE_TARGETS[3], v);
        }
        if (properties.size() >= GCODE_TARGETS.size()) {
            break;
        }
    }
    QMap<QString, QString> output = {
        {"printer", properties[GCODE_TARGETS[2]]},
        {"filament", properties[GCODE_TARGETS[4]].replace("\"", "")},
        {"filamentType", properties[GCODE_TARGETS[1]]},
        {"printSettings", properties[GCODE_TARGETS[5]]},
        {"weight", (properties.contains(GCODE_TARGETS[0])) ? properties[GCODE_TARGETS[0]] : properties[GCODE_TARGETS[6]]},
        {"duration", properties[GCODE_TARGETS[3]]},
    };
    return output;
}

QMap<QString, QString> GCodeParser::parseBGCode(QVector<QString> lines) {
    QMap<QString, QString> properties = QMap<QString, QString>();
    qDebug() << lines.size();
    for (int i = 0; i < min(lines.size(), 2000); i++) {
        QString line = lines[i];
        for (int p = 0; p < BGCODE_TARGETS.size(); p++) {
            if (line.contains(BGCODE_TARGETS[p] + "=")) {
                QStringList splitLine = line.split("=");
                splitLine.pop_front();
                QString v = splitLine.join("=").trimmed();
                properties.insert(BGCODE_TARGETS[p], v);
                break;
            }
        }
        if (properties.size() >= BGCODE_TARGETS.size()) break;
    }
    QMap<QString, QString> output = {
        {"printer", properties[BGCODE_TARGETS[2]]},
        {"filamentType", properties[BGCODE_TARGETS[1]]},
        {"weight", properties[BGCODE_TARGETS[0]]},
        {"duration", properties[BGCODE_TARGETS[3]]},
    };
    return output;
}

QVector<QString> GCodeParser::readGCode(QFile f) {
    if (f.open(QFile::OpenModeFlag::ReadOnly)) {
        QString plainText;
        if (f.size() > LINE_COUNT*60*2) {
            f.seek(0);
            bool isBambu = false;
            QByteArray firstFewData = f.read(10*60);
            QStringList firstFewLines = QString(firstFewData).split("\n");
            for (int i = 0; i < firstFewLines.size(); i++) {
                QString line = firstFewLines[i];
                if (line.startsWith("; model printing time:")) {
                    isBambu = true;
                    break;
                }
                if (line.startsWith("; THUMBNAIL_BLOCK_START")) {
                    isBambu = false;
                    break;
                }
            }
            if (isBambu) {
                f.seek(0);
                QByteArray firstData = f.read(LINE_COUNT * 20);
                f.seek(LINE_COUNT*20 + 20000);
                QByteArray secondData = f.read(LINE_COUNT * 40);
                f.seek(f.size() - LINE_COUNT*60); //End of file
                QByteArray endData = f.read(LINE_COUNT * 60);
                plainText = QString(firstData) + "\n" +QString(secondData)+ "\n" + QString(endData);
            } else {
                f.seek(f.size() - LINE_COUNT*60);
                QByteArray data = f.read(LINE_COUNT * 60);
                plainText = QString(data);
            }

        } else {
            QByteArray data = f.read(f.size());
            plainText = QString(data);
        }
        QVector<QString> lines = plainText.split("\n");
        return lines;
    }
    return QVector<QString>();
}

QVector<QString> GCodeParser::readBGCode(QFile f) {
    if (f.open(QFile::OpenModeFlag::ReadOnly)) {
        QByteArray data;
        data = f.read(min(f.size(), BLINE_COUNT*60));
        QString plainText = QString(data);
        plainText.replace(QChar(0xFFFD), "\n");
        QVector<QString> lines = plainText.split("\n");
        return lines;
    }
    return QVector<QString>();
}
