#include "videotools.h"

#define serializeData(fp, data, size) fp.append(data, size)

static QByteArray getMJPEGHeader(int width, int height, uint32_t frames, uint32_t bytes, float fps)
{
    QByteArray fp;

    serializeData(fp, "RIFF", 4); // FOURCC fcc; - 0
    serializeLong(fp, 216 + (frames * 8) + bytes); // DWORD cb; size - updated on close - 1
    serializeData(fp, "AVI ", 4); // FOURCC fcc; - 2

    serializeData(fp, "LIST", 4); // FOURCC fcc; - 3
    serializeLong(fp, 192); // DWORD cb; - 4
    serializeData(fp, "hdrl", 4); // FOURCC fcc; - 5

    serializeData(fp, "avih", 4); // FOURCC fcc; - 6
    serializeLong(fp, 56); // DWORD cb; - 7
    serializeLong(fp, (!roundf(fps)) ? 0 : roundf(1000000 / fps)); // DWORD dwMicroSecPerFrame; micros - updated on close - 8
    serializeLong(fp, (!frames) ? 0 : roundf((((frames * 8) + bytes) * fps) / frames)); // DWORD dwMaxBytesPerSec; updated on close - 9
    serializeLong(fp, 4); // DWORD dwPaddingGranularity; - 10
    serializeLong(fp, 0); // DWORD dwFlags; - 11
    serializeLong(fp, frames); // DWORD dwTotalFrames; frames - updated on close - 12
    serializeLong(fp, 0); // DWORD dwInitialFrames; - 13
    serializeLong(fp, 1); // DWORD dwStreams; - 14
    serializeLong(fp, 0); // DWORD dwSuggestedBufferSize; - 15
    serializeLong(fp, width); // DWORD dwWidth; width - updated on close - 16
    serializeLong(fp, height); // DWORD dwHeight; height - updated on close - 17
    serializeLong(fp, 1000); // DWORD dwScale; - 18
    serializeLong(fp, roundf(fps * 1000)); // DWORD dwRate; rate - updated on close - 19
    serializeLong(fp, 0); // DWORD dwStart; - 20
    serializeLong(fp, (!roundf(fps)) ? 0 : roundf((frames * 1000) / fps)); // DWORD dwLength; length - updated on close - 21

    serializeData(fp, "LIST", 4); // FOURCC fcc; - 22
    serializeLong(fp, 116); // DWORD cb; - 23
    serializeData(fp, "strl", 4); // FOURCC fcc; - 24

    serializeData(fp, "strh", 4); // FOURCC fcc; - 25
    serializeLong(fp, 56); // DWORD cb; - 26
    serializeData(fp, "vids", 4); // FOURCC fccType; - 27
    serializeData(fp, "MJPG", 4); // FOURCC fccHandler; - 28
    serializeLong(fp, 0); // DWORD dwFlags; - 29
    serializeWord(fp, 0); // WORD wPriority; - 30
    serializeWord(fp, 0); // WORD wLanguage; - 30.5
    serializeLong(fp, 0); // DWORD dwInitialFrames; - 31
    serializeLong(fp, 1000); // DWORD dwScale; - 32
    serializeLong(fp, roundf(fps * 1000)); // DWORD dwRate; rate - updated on close - 33
    serializeLong(fp, 0); // DWORD dwStart; - 34
    serializeLong(fp, (!roundf(fps)) ? 0 : roundf((frames * 1000) / fps)); // DWORD dwLength; length - updated on close - 35
    serializeLong(fp, 0); // DWORD dwSuggestedBufferSize; - 36
    serializeLong(fp, 10000); // DWORD dwQuality; - 37
    serializeLong(fp, 0); // DWORD dwSampleSize; - 38
    serializeWord(fp, 0); // short int left; - 39
    serializeWord(fp, 0); // short int top; - 39.5
    serializeWord(fp, 0); // short int right; - 40
    serializeWord(fp, 0); // short int bottom; - 40.5

    serializeData(fp, "strf", 4); // FOURCC fcc; - 41
    serializeLong(fp, 40); // DWORD cb; - 42
    serializeLong(fp, 40); // DWORD biSize; - 43
    serializeLong(fp, width); // LONG biWidth; width - updated on close - 44
    serializeLong(fp, height); // LONG biHeight; height - updated on close - 45
    serializeWord(fp, 1); // WORD biPlanes; - 46
    serializeWord(fp, 24); // WORD biBitCount; - 46.5
    serializeData(fp, "MJPG", 4); // DWORD biCompression; - 47
    serializeLong(fp, 0); // DWORD biSizeImage; - 48
    serializeLong(fp, 0); // LONG biXPelsPerMeter; - 49
    serializeLong(fp, 0); // LONG biYPelsPerMeter; - 50
    serializeLong(fp, 0); // DWORD biClrUsed; - 51
    serializeLong(fp, 0); // DWORD biClrImportant; - 52

    serializeData(fp, "LIST", 4); // FOURCC fcc; - 53
    serializeLong(fp, 4 + (frames * 8) + bytes); // DWORD cb; movi - updated on close - 54
    serializeData(fp, "movi", 4); // FOURCC fcc; - 55

    return fp;
}

static QByteArray addMJPEG(uint32_t *frames, uint32_t *bytes, const QPixmap &pixmap)
{
    QByteArray fp;

    serializeData(fp, "00dc", 4); // FOURCC fcc;
    *frames += 1;

    QByteArray data;
    QBuffer buffer(&data);
    buffer.open(QIODevice::WriteOnly); // always return true
    pixmap.save(&buffer, "JPG"); // always return true
    buffer.close();

    int pad = (((data.size() + 3) / 4) * 4) - data.size();
    serializeLong(fp, data.size() + pad); // DWORD cb;
    serializeData(fp, data.data(), data.size());
    serializeData(fp, "\0\0", pad);
    *bytes += data.size() + pad;

    return fp;
}

static bool getMaxSizeAndAvgMsDelta(QFile *imageWriterFile, int *avgM, int *maxW, int *maxH)
{
    QProgressDialog progress(QObject::tr("Reading File..."), QObject::tr("Cancel"), imageWriterFile->pos(), imageWriterFile->size(), Core::ICore::dialogParent(),
        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
    progress.setWindowModality(Qt::ApplicationModal);

    QDataStream stream(imageWriterFile);
    stream.setByteOrder(QDataStream::LittleEndian);

    if(stream.atEnd())
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            QObject::tr("Reading File"),
            QObject::tr("No frames found!"));

        return false;
    }

    while(!stream.atEnd())
    {
        progress.setValue(imageWriterFile->pos());

        int M, W, H, BPP;

        stream >> M;
        stream >> W;
        stream >> H;
        stream >> BPP;

        if((M < 0) || (W < 0) || (H < 0) || (BPP < 0))
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QObject::tr("Reading File"),
                QObject::tr("File is corrupt!"));

            return false;
        }

        int size = ((getImageSize(W, H, BPP) + 15) / 16) * 16;

        if(stream.skipRawData(size) != size)
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QObject::tr("Reading File"),
                QObject::tr("File is corrupt!"));

            return false;
        }

        *avgM = (*avgM != -1) ? ((*avgM + M) / 2) : M;
        *maxW = qMax(*maxW, W);
        *maxH = qMax(*maxH, H);

        if(progress.wasCanceled())
        {
            return false;
        }
    }

    return true;
}

static bool convertImageWriterFileToMjpegVideoFile(QFile *mjpegVideoFile, uint32_t *frames, uint32_t *bytes, QFile *imageWriterFile, int avgM, int maxW, int maxH)
{
    QProgressDialog progress(QObject::tr("Transcoding File..."), QObject::tr("Cancel"), imageWriterFile->pos(), imageWriterFile->size(), Core::ICore::dialogParent(),
        Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::CustomizeWindowHint |
        (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowType(0)));
    progress.setWindowModality(Qt::ApplicationModal);

    QDataStream stream(imageWriterFile);
    stream.setByteOrder(QDataStream::LittleEndian);

    if(stream.atEnd())
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            QObject::tr("Transcoding File"),
            QObject::tr("No frames found!"));

        return false;
    }

    for(long long time_counter = 0; !stream.atEnd(); time_counter += 0)
    {
        progress.setValue(imageWriterFile->pos());

        int M, W, H, BPP;

        stream >> M;
        stream >> W;
        stream >> H;
        stream >> BPP;

        if((M < 0) || (W < 0) || (H < 0) || (BPP < 0))
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QObject::tr("Transcoding File"),
                QObject::tr("File is corrupt!"));

            return false;
        }

        int size = getImageSize(W, H, BPP);

        QByteArray data(size, 0);

        if(stream.readRawData(data.data(), size) != size)
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QObject::tr("Transcoding File"),
                QObject::tr("File is corrupt!"));

            return false;
        }

        QPixmap pixmap = getImageFromData(data, W, H, BPP);

        size = 16 - (size % 16);

        if((size != 16) && (stream.skipRawData(size) != size))
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QObject::tr("Transcoding File"),
                QObject::tr("File is corrupt!"));

            return false;
        }

        QPixmap image(maxW, maxH);

        image.fill(Qt::black);

        QPainter painter;

        if(!painter.begin(&image))
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QObject::tr("Transcoding File"),
                QObject::tr("Painter Failed!"));

            return false;
        }

        painter.drawPixmap((maxW - pixmap.width()) / 2, (maxH - pixmap.height()) / 2, pixmap.scaled(maxW, maxH, Qt::KeepAspectRatio));

        if(!painter.end())
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QObject::tr("Transcoding File"),
                QObject::tr("Painter Failed!"));

            return false;
        }

        for(long long time_counter_2 = time_counter + avgM; time_counter < time_counter_2; time_counter += M)
        {
            QByteArray jpeg = addMJPEG(frames, bytes, image);

            if(mjpegVideoFile->write(jpeg) != jpeg.size())
            {
                QMessageBox::critical(Core::ICore::dialogParent(),
                    QObject::tr("Transcoding File"),
                    QObject::tr("Failed to write!"));

                return false;
            }

            if(progress.wasCanceled())
            {
                return false;
            }
        }
    }

    return true;
}

static QString handleImageWriterFiles(const QString &path)
{
    QFile file(path);

    if(file.open(QIODevice::ReadOnly))
    {
        QByteArray data = file.read(16);

        if((file.error() == QFile::NoError) && (data.size() == 16) && (!memcmp(data.data(), "OMV IMG STR V", 13)) && (data.at(14) == '.'))
        {
            char major = *(data.data() + 13);
            char minor = *(data.data() + 15);

            if(isdigit(major) && isdigit(minor))
            {
                int version = ((major - '0') * 10) + (minor - '0');

                if(version == 10)
                {
                    QFile tempFile(QDir::tempPath() + QDir::separator() + QFileInfo(file).completeBaseName() + QStringLiteral(".mjpeg"));

                    if(tempFile.open(QIODevice::WriteOnly))
                    {
                        int avgM = -1, maxW = 0, maxH = 0;

                        if(getMaxSizeAndAvgMsDelta(&file, &avgM, &maxW, &maxH))
                        {
                            if(file.seek(16))
                            {
                                QByteArray header = getMJPEGHeader(maxW, maxH, 0, 0, 0);

                                if(tempFile.write(header) == header.size())
                                {
                                    uint32_t frames = 0, bytes = 0;

                                    if(convertImageWriterFileToMjpegVideoFile(&tempFile, &frames, &bytes, &file, avgM, maxW, maxH))
                                    {
                                        if(tempFile.seek(0))
                                        {
                                            header = getMJPEGHeader(maxW, maxH, frames, bytes, 1000.0 / avgM);

                                            if(tempFile.write(header) == header.size())
                                            {
                                                return QFileInfo(tempFile).canonicalFilePath();
                                            }
                                            else
                                            {
                                                QMessageBox::critical(Core::ICore::dialogParent(),
                                                    QObject::tr("Transcoder"),
                                                    QObject::tr("Failed to write header again!"));
                                            }
                                        }
                                        else
                                        {
                                            QMessageBox::critical(Core::ICore::dialogParent(),
                                                QObject::tr("Transcoder"),
                                                QObject::tr("Seek failed!"));
                                        }
                                    }
                                }
                                else
                                {
                                    QMessageBox::critical(Core::ICore::dialogParent(),
                                        QObject::tr("Transcoder"),
                                        QObject::tr("Failed to write header!"));
                                }
                            }
                            else
                            {
                                QMessageBox::critical(Core::ICore::dialogParent(),
                                    QObject::tr("Transcoder"),
                                    QObject::tr("Seek failed!"));
                            }
                        }
                    }
                    else
                    {
                        QMessageBox::critical(Core::ICore::dialogParent(),
                            QObject::tr("Transcoder"),
                            QObject::tr("Error: %L1!").arg(tempFile.errorString()));
                    }
                }
                else
                {
                    QMessageBox::critical(Core::ICore::dialogParent(),
                        QObject::tr("Transcoder"),
                        QObject::tr("Unsupported OpenMV ImageWriter File version!"));
                }
            }
            else
            {
                return path; // Not an ImageWriter file.
            }
        }
        else if(file.error() != QFile::NoError)
        {
            QMessageBox::critical(Core::ICore::dialogParent(),
                QObject::tr("Transcoder"),
                QObject::tr("Error: %L1!").arg(file.errorString()));
        }
        else
        {
            return path; // Not an ImageWriter file.
        }
    }
    else
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            QObject::tr("Transcoder"),
            QObject::tr("Error: %L1!").arg(file.errorString()));
    }

    return QString();
}

static QString getInputFormats()
{
    QString command;
    Utils::SynchronousProcess process;
    Utils::SynchronousProcessResponse response;
    process.setTimeoutS(10);
    process.setProcessChannelMode(QProcess::MergedChannels);
    response.result = Utils::SynchronousProcessResponse::FinishedError;

    if(Utils::HostOsInfo::isWindowsHost())
    {
        command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/windows/bin/ffmpeg.exe")));
        response = process.run(command, QStringList()
            << QStringLiteral("-hide_banner")
            << QStringLiteral("-muxers"));
    }
    else if(Utils::HostOsInfo::isMacHost())
    {
        command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/mac/ffmpeg")));
        response = process.run(command, QStringList()
            << QStringLiteral("-hide_banner")
            << QStringLiteral("-muxers"));
    }
    else if(Utils::HostOsInfo::isLinuxHost())
    {
        if(QSysInfo::buildCpuArchitecture() == QStringLiteral("i386"))
        {
            command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-x86/ffmpeg")));
            response = process.run(command, QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-muxers"));
        }
        else if(QSysInfo::buildCpuArchitecture() == QStringLiteral("x86_64"))
        {
            command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-x86_64/ffmpeg")));
            response = process.run(command, QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-muxers"));
        }
        else if(QSysInfo::buildCpuArchitecture() == QStringLiteral("arm"))
        {
            command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-arm/ffmpeg")));
            response = process.run(command, QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-muxers"));
        }
    }

    if(response.result == Utils::SynchronousProcessResponse::Finished)
    {
        QStringList list, in = response.stdOut.split(QRegularExpression(QStringLiteral("\n|\r\n|\r")), QString::SkipEmptyParts);

        foreach(const QString &string, in)
        {
            QRegularExpressionMatch match = QRegularExpression(QStringLiteral("\\s+E\\s+(\\w+)\\s+(.+)")).match(string);

            if(match.hasMatch())
            {
                list.append(QString(QStringLiteral("%1 (*.%2)")).arg(match.captured(2).replace(QLatin1Char('('), QLatin1Char('[')).replace(QLatin1Char(')'), QLatin1Char(']'))).arg(match.captured(1)));
            }
        }

        return list.join(QStringLiteral(";;"));
    }
    else
    {
        QMessageBox box(QMessageBox::Warning, QObject::tr("Get Input Formats"), QObject::tr("Query failed!"), QMessageBox::Ok, Core::ICore::dialogParent(),
            Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
            (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
        box.setDetailedText(response.stdOut);
        box.setInformativeText(response.exitMessage(command, process.timeoutS()));
        box.setDefaultButton(QMessageBox::Ok);
        box.setEscapeButton(QMessageBox::Cancel);

        return QString();
    }
}

static QString getOutputFormats()
{
    QString command;
    Utils::SynchronousProcess process;
    Utils::SynchronousProcessResponse response;
    process.setTimeoutS(10);
    process.setProcessChannelMode(QProcess::MergedChannels);
    response.result = Utils::SynchronousProcessResponse::FinishedError;

    if(Utils::HostOsInfo::isWindowsHost())
    {
        command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/windows/bin/ffmpeg.exe")));
        response = process.run(command, QStringList()
            << QStringLiteral("-hide_banner")
            << QStringLiteral("-demuxers"));
    }
    else if(Utils::HostOsInfo::isMacHost())
    {
        command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/mac/ffmpeg")));
        response = process.run(command, QStringList()
            << QStringLiteral("-hide_banner")
            << QStringLiteral("-demuxers"));
    }
    else if(Utils::HostOsInfo::isLinuxHost())
    {
        if(QSysInfo::buildCpuArchitecture() == QStringLiteral("i386"))
        {
            command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-x86/ffmpeg")));
            response = process.run(command, QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-demuxers"));
        }
        else if(QSysInfo::buildCpuArchitecture() == QStringLiteral("x86_64"))
        {
            command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-x86_64/ffmpeg")));
            response = process.run(command, QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-demuxers"));
        }
        else if(QSysInfo::buildCpuArchitecture() == QStringLiteral("arm"))
        {
            command = QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-arm/ffmpeg")));
            response = process.run(command, QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-demuxers"));
        }
    }

    if(response.result == Utils::SynchronousProcessResponse::Finished)
    {
        QStringList list, in = response.stdOut.split(QRegularExpression(QStringLiteral("\n|\r\n|\r")), QString::SkipEmptyParts);

        foreach(const QString &string, in)
        {
            QRegularExpressionMatch match = QRegularExpression(QStringLiteral("\\s+D\\s+(\\w+)\\s+(.+)")).match(string);

            if(match.hasMatch())
            {
                list.append(QString(QStringLiteral("%1 (*.%2)")).arg(match.captured(2).replace(QLatin1Char('('), QLatin1Char('[')).replace(QLatin1Char(')'), QLatin1Char(']'))).arg(match.captured(1)));
            }
        }

        return list.join(QStringLiteral(";;"));
    }
    else
    {
        QMessageBox box(QMessageBox::Warning, QObject::tr("Get Input Formats"), QObject::tr("Query failed!"), QMessageBox::Ok, Core::ICore::dialogParent(),
            Qt::MSWindowsFixedSizeDialogHint | Qt::WindowTitleHint | Qt::WindowSystemMenuHint |
            (Utils::HostOsInfo::isMacHost() ? Qt::WindowType(0) : Qt::WindowCloseButtonHint));
        box.setDetailedText(response.stdOut);
        box.setInformativeText(response.exitMessage(command, process.timeoutS()));
        box.setDefaultButton(QMessageBox::Ok);
        box.setEscapeButton(QMessageBox::Cancel);

        return QString();
    }
}

static bool convertVideoFile(const QString &dst, const QString &src)
{
    bool result;

    if(Utils::HostOsInfo::isWindowsHost())
    {
        result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/windows/bin/ffmpeg.exe"))), QStringList()
            << QStringLiteral("-hide_banner")
            << QStringLiteral("-i")
            << QDir::cleanPath(QDir::toNativeSeparators(src))
            << QDir::cleanPath(QDir::toNativeSeparators(dst)));
    }
    else if(Utils::HostOsInfo::isMacHost())
    {
        result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/mac/ffmpeg"))), QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-i")
                << QDir::cleanPath(QDir::toNativeSeparators(src))
                << QDir::cleanPath(QDir::toNativeSeparators(dst)));
    }
    else if(Utils::HostOsInfo::isLinuxHost())
    {
        if(QSysInfo::buildCpuArchitecture() == QStringLiteral("i386"))
        {
            result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-x86/ffmpeg"))), QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-i")
                << QDir::cleanPath(QDir::toNativeSeparators(src))
                << QDir::cleanPath(QDir::toNativeSeparators(dst)));
        }
        else if(QSysInfo::buildCpuArchitecture() == QStringLiteral("x86_64"))
        {
            result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-x86_64/ffmpeg"))), QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-i")
                << QDir::cleanPath(QDir::toNativeSeparators(src))
                << QDir::cleanPath(QDir::toNativeSeparators(dst)));
        }
        else if(QSysInfo::buildCpuArchitecture() == QStringLiteral("arm"))
        {
            result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-arm/ffmpeg"))), QStringList()
                << QStringLiteral("-hide_banner")
                << QStringLiteral("-i")
                << QDir::cleanPath(QDir::toNativeSeparators(src))
                << QDir::cleanPath(QDir::toNativeSeparators(dst)));
        }
    }

    if(!result)
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            QObject::tr("Convert Video"),
            QObject::tr("Failed to launch ffmpeg!"));
    }

    return result;
}

static bool playVideoFile(const QString &path)
{
    bool result;

    if(Utils::HostOsInfo::isWindowsHost())
    {
        result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/windows/bin/ffplay.exe"))), QStringList()
            << QStringLiteral("-hide_banner")
            << QDir::cleanPath(QDir::toNativeSeparators(path)));
    }
    else if(Utils::HostOsInfo::isMacHost())
    {
        result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/mac/ffplay"))), QStringList()
            << QStringLiteral("-hide_banner")
            << QDir::cleanPath(QDir::toNativeSeparators(path)));
    }
    else if(Utils::HostOsInfo::isLinuxHost())
    {
        if(QSysInfo::buildCpuArchitecture() == QStringLiteral("i386"))
        {
            result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-x86/ffplay"))), QStringList()
                << QStringLiteral("-hide_banner")
                << QDir::cleanPath(QDir::toNativeSeparators(path)));
        }
        else if(QSysInfo::buildCpuArchitecture() == QStringLiteral("x86_64"))
        {
            result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-x86_64/ffplay"))), QStringList()
                << QStringLiteral("-hide_banner")
                << QDir::cleanPath(QDir::toNativeSeparators(path)));
        }
        else if(QSysInfo::buildCpuArchitecture() == QStringLiteral("arm"))
        {
            result = QProcess::startDetached(QDir::cleanPath(QDir::toNativeSeparators(Core::ICore::resourcePath() + QStringLiteral("/ffmpeg/linux-arm/ffplay"))), QStringList()
                << QStringLiteral("-hide_banner")
                << QDir::cleanPath(QDir::toNativeSeparators(path)));
        }
    }

    if(!result)
    {
        QMessageBox::critical(Core::ICore::dialogParent(),
            QObject::tr("Play Video"),
            QObject::tr("Failed to launch ffplay!"));
    }

    return result;
}

void convertVideoFileAction(const QString &drivePath)
{
    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QStringLiteral(VIDEO_SETTINGS_GROUP));

    QString src =
        QFileDialog::getOpenFileName(Core::ICore::dialogParent(), QObject::tr("Convert Video Source"),
            settings->value(QStringLiteral(LAST_CONVERT_VIDEO_SRC_PATH), drivePath.isEmpty() ? QDir::homePath() : drivePath).toString(),
            QObject::tr("Video Files (*.*);;OpenMV ImageWriter Files (*.bin);;") + getInputFormats());

    if(!src.isEmpty())
    {
        QString dst =
            QFileDialog::getSaveFileName(Core::ICore::dialogParent(), QObject::tr("Convert Video Output"),
                settings->value(QStringLiteral(LAST_CONVERT_VIDEO_DST_PATH), QDir::homePath()).toString(),
                QObject::tr("Video Files (*.*);;") + getOutputFormats());

        if(!dst.isEmpty())
        {
            QString tempSrc = handleImageWriterFiles(src);

            if((!tempSrc.isEmpty()) && convertVideoFile(dst, tempSrc))
            {
                settings->setValue(QStringLiteral(LAST_CONVERT_VIDEO_SRC_PATH), src);
                settings->setValue(QStringLiteral(LAST_CONVERT_VIDEO_DST_PATH), dst);
            }
        }
    }

    settings->endGroup();
}

void playVideoFileAction(const QString &drivePath)
{
    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QStringLiteral(VIDEO_SETTINGS_GROUP));

    QString path =
        QFileDialog::getOpenFileName(Core::ICore::dialogParent(), QObject::tr("Play Video"),
            settings->value(QStringLiteral(LAST_PLAY_VIDEO_PATH), drivePath.isEmpty() ? QDir::homePath() : drivePath).toString(),
            QObject::tr("Video Files (*.*);;OpenMV ImageWriter Files (*.bin);;") + getInputFormats());

    if(!path.isEmpty())
    {
        QString tempPath = handleImageWriterFiles(path);

        if((!tempPath.isEmpty()) && playVideoFile(tempPath))
        {
            settings->setValue(QStringLiteral(LAST_PLAY_VIDEO_PATH), path);
        }
    }

    settings->endGroup();
}

void saveVideoFile(const QString &srcPath)
{
    QSettings *settings = ExtensionSystem::PluginManager::settings();
    settings->beginGroup(QStringLiteral(VIDEO_SETTINGS_GROUP));

    QString dst =
        QFileDialog::getSaveFileName(Core::ICore::dialogParent(), QObject::tr("Save Video"),
            settings->value(QStringLiteral(LAST_SAVE_VIDEO_PATH), QDir::homePath()).toString(),
            QObject::tr("Video Files (*.*);;") + getOutputFormats());

    if(!dst.isEmpty())
    {
        QString tempSrc = handleImageWriterFiles(srcPath);

        if((!tempSrc.isEmpty()) && convertVideoFile(dst, tempSrc))
        {
            settings->setValue(QStringLiteral(LAST_SAVE_VIDEO_PATH), dst);
        }
    }

    settings->endGroup();
}
