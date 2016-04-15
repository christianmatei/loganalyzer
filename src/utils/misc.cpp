/*
 * Copyright (C) 2016 Patrizio Bekerle -- http://www.bekerle.com
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 */

#include <QProcess>
#include <QDesktopServices>
#include <QDir>
#include <QUrl>
#include <QRegularExpression>
#include <QtCore/QCoreApplication>
#include <QtWidgets/QApplication>
#include <cmath>
#include "misc.h"

#ifdef Q_OS_WIN
#include <windows.h>
#endif

static struct { const char *source; const char *comment; } units[] = {
        QT_TRANSLATE_NOOP3("misc", "B", "bytes"),
        QT_TRANSLATE_NOOP3("misc", "kB", "kibibytes (1024 bytes)"),
        QT_TRANSLATE_NOOP3("misc", "MB", "mebibytes (1024 kibibytes)"),
        QT_TRANSLATE_NOOP3("misc", "GB", "gibibytes (1024 mibibytes)"),
        QT_TRANSLATE_NOOP3("misc", "TB", "tebibytes (1024 gibibytes)"),
        QT_TRANSLATE_NOOP3("misc", "PB", "pebibytes (1024 tebibytes)"),
        QT_TRANSLATE_NOOP3("misc", "EB", "exbibytes (1024 pebibytes)")
};

/**
 * Open the given path with an appropriate application
 * (thank you to qBittorrent for the inspiration)
 */
void Utils::Misc::openPath(const QString& absolutePath)
{
    const QString path = QDir::fromNativeSeparators(absolutePath);
    // Hack to access samba shares with QDesktopServices::openUrl
    if (path.startsWith("//"))
        QDesktopServices::openUrl(QDir::toNativeSeparators("file:" + path));
    else
        QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

/**
 * Opens the parent directory of the given path with a file manager and select
 * (if possible) the item at the given path
 * (thank you to qBittorrent for the inspiration)
 */
void Utils::Misc::openFolderSelect(const QString& absolutePath)
{
    const QString path = QDir::fromNativeSeparators(absolutePath);
#ifdef Q_OS_WIN
    if (QFileInfo(path).exists()) {
        // Syntax is: explorer /select, "C:\Folder1\Folder2\file_to_select"
        // Dir separators MUST be win-style slashes

        // QProcess::startDetached() has an obscure bug. If the path has
        // no spaces and a comma(and maybe other special characters) it doesn't
        // get wrapped in quotes. So explorer.exe can't find the correct path
        // anddisplays the default one. If we wrap the path in quotes and pass
        // it to QProcess::startDetached() explorer.exe still shows the default
        // path. In this case QProcess::startDetached() probably puts its
        // own quotes around ours.

        STARTUPINFO startupInfo;
        ::ZeroMemory(&startupInfo, sizeof(startupInfo));
        startupInfo.cb = sizeof(startupInfo);

        PROCESS_INFORMATION processInfo;
        ::ZeroMemory(&processInfo, sizeof(processInfo));

        QString cmd = QString("explorer.exe /select,\"%1\"")
            .arg(QDir::toNativeSeparators(absolutePath));
        LPWSTR lpCmd = new WCHAR[cmd.size() + 1];
        cmd.toWCharArray(lpCmd);
        lpCmd[cmd.size()] = 0;

        bool ret = ::CreateProcessW(
            NULL, lpCmd, NULL, NULL, FALSE, 0, NULL, NULL,
            &startupInfo, &processInfo);
        delete [] lpCmd;

        if (ret) {
            ::CloseHandle(processInfo.hProcess);
            ::CloseHandle(processInfo.hThread);
        }
    } else {
        // If the item to select doesn't exist, try to open its parent
        openPath(path.left(path.lastIndexOf("/")));
    }
#elif defined(Q_OS_UNIX) && !defined(Q_OS_MAC)
    if (QFileInfo(path).exists()) {
        QProcess proc;
        QString output;
        proc.start(
                "xdg-mime",
                QStringList() << "query" << "default" << "inode/directory");
        proc.waitForFinished();
        output = proc.readLine().simplified();
        if (output == "dolphin.desktop" ||
                output == "org.kde.dolphin.desktop") {
            proc.startDetached(
                    "dolphin",
                    QStringList() << "--select"
                    << QDir::toNativeSeparators(path));
        } else if (output == "nautilus.desktop" ||
                    output == "org.gnome.Nautilus.desktop" ||
                    output == "nautilus-folder-handler.desktop") {
            proc.startDetached(
                    "nautilus",
                    QStringList() << "--no-desktop"
                    << QDir::toNativeSeparators(path));
        } else if (output == "caja-folder-handler.desktop") {
            proc.startDetached(
                    "caja",
                    QStringList() << "--no-desktop"
                    << QDir::toNativeSeparators(path));
        } else if (output == "nemo.desktop") {
            proc.startDetached(
                    "nemo",
                    QStringList() << "--no-desktop"
                    << QDir::toNativeSeparators(path));
        } else if (output == "konqueror.desktop" ||
                   output == "kfmclient_dir.desktop") {
            proc.startDetached(
                    "konqueror",
                    QStringList() << "--select"
                    << QDir::toNativeSeparators(path));
        } else {
            openPath(path.left(path.lastIndexOf("/")));
        }
    } else {
        // if the item to select doesn't exist, try to open its parent
        openPath(path.left(path.lastIndexOf("/")));
    }
#else
    openPath(path.left(path.lastIndexOf("/")));
#endif
}

/**
 * Removes a string from the start if it starts with it
 */
QString Utils::Misc::removeIfStartsWith(QString text, QString removeString) {
    if (text.startsWith(removeString)) {
        text.remove(QRegularExpression(
                "^" + QRegularExpression::escape(removeString)));
    }

    return text;
}

/**
 * Removes a string from the end if it ends with it
 */
QString Utils::Misc::removeIfEndsWith(QString text, QString removeString) {
    if (text.endsWith(removeString)) {
        text.remove(QRegularExpression(
                QRegularExpression::escape(removeString) + "$"));
    }

    return text;
}

/**
 * Adds a string to the beginning of a string if it doesn't start with it
 */
QString Utils::Misc::prependIfDoesNotStartWith(
        QString text, QString startString) {
    if (!text.startsWith(startString)) {
        text.prepend(startString);
    }

    return text;
}

/**
 * Adds a string to the end of a string if it doesn't end with it
 */
QString Utils::Misc::appendIfDoesNotEndWith(
        QString text, QString endString) {
    if (!text.endsWith(endString)) {
        text.append(endString);
    }

    return text;
}

/**
 * (thank you to qBittorrent for the inspiration)
 */
QString Utils::Misc::unitString(Utils::Misc::SizeUnit unit)
{
    return QCoreApplication::translate(
            "misc",
            units[static_cast<int>(unit)].source,
            units[static_cast<int>(unit)].comment);
}

/**
 * Returns best user-friendly storage unit (B, KiB, MiB, GiB, TiB, ...)
 *
 * use Binary prefix standards from IEC 60027-2
 * see http://en.wikipedia.org/wiki/Kilobyte
 * value must be given in bytes
 * to send numbers instead of strings with suffixes
 * (thank you to qBittorrent for the inspiration)
 */
bool Utils::Misc::friendlyUnit(
        qint64 sizeInBytes, qreal &val, Utils::Misc::SizeUnit &unit)
{
    if (sizeInBytes < 0) return false;

    int i = 0;
    qreal rawVal = static_cast<qreal>(sizeInBytes);

    while ((rawVal >= 1024.) && (i <= static_cast<int>(SizeUnit::ExbiByte))) {
        rawVal /= 1024.;
        ++i;
    }
    val = rawVal;
    unit = static_cast<SizeUnit>(i);
    return true;
}

/**
 * (thank you to qBittorrent for the inspiration)
 */
QString Utils::Misc::friendlyUnit(qint64 bytesValue, bool isSpeed)
{
    SizeUnit unit;
    qreal friendlyVal;
    if (!friendlyUnit(bytesValue, friendlyVal, unit)) {
        return QCoreApplication::translate("misc", "Unknown", "Unknown (size)");
    }
    QString ret;
    if (unit == SizeUnit::Byte)
        ret = QString::number(bytesValue) + " " + unitString(unit);
    else
        ret = fromDouble(friendlyVal, 1) + " " + unitString(unit);
    if (isSpeed)
        ret += QCoreApplication::translate("misc", "/s", "per second");
    return ret;
}

/**
 * To send numbers instead of strings with suffixes
 * (thank you to qBittorrent for the inspiration)
 */
QString Utils::Misc::fromDouble(double n, int precision)
{
    /* HACK because QString rounds up. Eg QString::number(0.999*100.0, 'f' ,1) == 99.9
    ** but QString::number(0.9999*100.0, 'f' ,1) == 100.0 The problem manifests when
    ** the number has more digits after the decimal than we want AND the digit after
    ** our 'wanted' is >= 5. In this case our last digit gets rounded up. So for each
    ** precision we add an extra 0 behind 1 in the below algorithm. */

    double prec = std::pow(10.0, precision);
    return QLocale::system().toString(
            std::floor(n * prec) / prec, 'f', precision);
}
