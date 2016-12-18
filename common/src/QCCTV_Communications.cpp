/*
 * Copyright (c) 2016 Alex Spataru
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE
 */

#include "QCCTV_CRC32.h"
#include "QCCTV_Communications.h"

#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>

static QCCTV_CRC32 crc32;

/* Stream packet keys */
static const QString KEY_FPS        = "fps";
static const QString KEY_ZOOM       = "zoom";
static const QString KEY_NAME       = "name";
static const QString KEY_GROUP      = "group";
static const QString KEY_STATUS     = "status";
static const QString KEY_RESOLUTION = "resolution";
static const QString KEY_FLASHLIGHT = "flashlight";
static const QString KEY_ZOOM_AVAIL = "zoomSupported";
static const QString KEY_AUTOREGRES = "autoRegulateResolution";

/* Command packet keys */
static const QString KEY_OLD_FPS = "o_fps";
static const QString KEY_NEW_FPS = "n_fps";
static const QString KEY_OLD_ZOOM = "o_zoom";
static const QString KEY_NEW_ZOOM = "n_zoom";
static const QString KEY_FOCUS_REQUEST  = "focus";
static const QString KEY_OLD_RESOLUTION = "o_res";
static const QString KEY_NEW_RESOLUTION = "n_res";
static const QString KEY_OLD_FLASHLIGHT = "o_flashlight";
static const QString KEY_NEW_FLASHLIGHT = "n_flashlight";
static const QString KEY_OLD_AUTOREGRES = "o_autoRegulateResolution";
static const QString KEY_NEW_AUTOREGRES = "n_autoRegulateResolution";

/**
 * Initializes the default values for the given stream \a packet
 */
void QCCTV_InitStream (QCCTV_StreamPacket* packet)
{
    if (packet) {
        packet->fps = 18;
        packet->crc32 = 0;
        packet->zoom = 0;
        packet->supportsZoom = false;
        packet->cameraName = "Unknown";
        packet->cameraGroup = "Default";
        packet->flashlightEnabled = false;
        packet->resolution = QCCTV_Original;
        packet->autoRegulateResolution = true;
        packet->cameraStatus = QCCTV_CAMSTATUS_DEFAULT;
        packet->image = QCCTV_CreateStatusImage (QSize (640, 480),
                                                 "NO CAMERA IMAGE");
    }
}

/**
 * Initializes the variables of the given \a command packet with the
 * values in the given \a stream packet
 */
void QCCTV_InitCommand (QCCTV_CommandPacket* command,
                        QCCTV_StreamPacket* stream)
{
    if (command && stream) {
        command->focusRequest = false;
        command->newFps = stream->fps;
        command->oldFps = stream->fps;
        command->oldZoom = stream->zoom;
        command->newZoom = stream->zoom;
        command->oldResolution = stream->resolution;
        command->newResolution = stream->resolution;
        command->oldFlashlightEnabled = stream->flashlightEnabled;
        command->newFlashlightEnabled = stream->flashlightEnabled;
        command->oldAutoRegulateResolution = stream->autoRegulateResolution;
        command->newAutoRegulateResolution = stream->autoRegulateResolution;
    }
}

/**
 * Reads the given stream \a packet and generates the binary data that can be
 * sent through a network socket to a connected QCCTV Station
 */
QByteArray QCCTV_CreateStreamPacket (const QCCTV_StreamPacket& packet)
{
    /* Create initial JSON object */
    QJsonObject json;

    /* Add information */
    json.insert (KEY_FPS, packet.fps);
    json.insert (KEY_ZOOM, packet.zoom);
    json.insert (KEY_NAME, packet.cameraName);
    json.insert (KEY_GROUP, packet.cameraGroup);
    json.insert (KEY_STATUS, packet.cameraStatus);
    json.insert (KEY_RESOLUTION, packet.resolution);
    json.insert (KEY_ZOOM_AVAIL, packet.supportsZoom);
    json.insert (KEY_FLASHLIGHT, packet.flashlightEnabled);
    json.insert (KEY_AUTOREGRES, packet.autoRegulateResolution);

    /* Add raw image bytes */
    QCCTV_Resolution res = (QCCTV_Resolution) packet.resolution;
    QByteArray image = QCCTV_EncodeImage (packet.image, res);
    if (!image.isEmpty())
        json.insert ("image", QString (image.toBase64()));

    /* Convert JSON data to binary data */
    QByteArray data = QJsonDocument (json).toBinaryData();

    /* Add the cheksum at the start of the data */
    quint32 crc = crc32.compute (data);
    data.prepend ((crc & 0xff));
    data.prepend ((crc & 0xff00) >> 8);
    data.prepend ((crc & 0xff0000) >> 16);
    data.prepend ((crc & 0xff000000) >> 24);

    /* Return obtained data */
    return data;
}

/**
 * Reads the given \a binary data and updates the values if the given stream
 * \a packet structure
 *
 * This function shall return \c true on success, \c false on failure
 */
bool QCCTV_ReadStreamPacket (QCCTV_StreamPacket* packet,
                             const QByteArray& data)
{
    /* Packet pointer is invalid and/or data is empty */
    if (!packet || data.length() < 4)
        return false;

    /* Get the checksum */
    quint8 a = data.at (0);
    quint8 b = data.at (1);
    quint8 c = data.at (2);
    quint8 d = data.at (3);
    packet->crc32 = (a << 24) | (b << 16) | (c << 8) | (d & 0xff);

    /* Create byte array without checksum header */
    QByteArray stream = data;
    stream.remove (0, 4);

    /* Compare checksums (abort if they are different) */
    quint32 crc = crc32.compute (stream);
    if (packet->crc32 != crc)
        return false;

    /* Get JSON object from data */
    QJsonObject json = QJsonDocument::fromBinaryData (stream).object();
    if (json.isEmpty())
        return false;

    /* Get information from JSON object */
    packet->fps = json.value (KEY_FPS).toInt();
    packet->zoom = json.value (KEY_ZOOM).toInt();
    packet->cameraName = json.value (KEY_NAME).toString();
    packet->cameraStatus = json.value (KEY_STATUS).toInt();
    packet->cameraGroup = json.value (KEY_GROUP).toString();
    packet->resolution = json.value (KEY_RESOLUTION).toInt();
    packet->supportsZoom = json.value (KEY_ZOOM_AVAIL).toBool();
    packet->flashlightEnabled = json.value (KEY_FLASHLIGHT).toBool();
    packet->autoRegulateResolution = json.value (KEY_AUTOREGRES).toBool();

    /* Get camera image from object */
    QByteArray base64 = json.value ("image").toString().toUtf8();
    if (!base64.isEmpty())
        packet->image = QCCTV_DecodeImage (QByteArray::fromBase64 (base64));

    /* Return the status of the image */
    return !packet->image.isNull();
}

/**
 * Reads the given command \a packet and generates the binary data that can be
 * sent through a network socket to a connected QCCTV Camera
 */
QByteArray QCCTV_CreateCommandPacket (const QCCTV_CommandPacket& packet)
{
    QJsonObject json;
    json.insert (KEY_OLD_FPS, packet.oldFps);
    json.insert (KEY_NEW_FPS, packet.newFps);
    json.insert (KEY_OLD_ZOOM, packet.oldZoom);
    json.insert (KEY_NEW_ZOOM, packet.newZoom);
    json.insert (KEY_FOCUS_REQUEST, packet.focusRequest);
    json.insert (KEY_OLD_RESOLUTION, packet.oldResolution);
    json.insert (KEY_NEW_RESOLUTION, packet.newResolution);
    json.insert (KEY_OLD_FLASHLIGHT, packet.oldFlashlightEnabled);
    json.insert (KEY_NEW_FLASHLIGHT, packet.newFlashlightEnabled);
    json.insert (KEY_OLD_AUTOREGRES, packet.oldAutoRegulateResolution);
    json.insert (KEY_NEW_AUTOREGRES, packet.newAutoRegulateResolution);
    return QJsonDocument (json).toBinaryData();
}

/**
 * Reads the given \a binary data and updates the values if the given command
 * \a packet structure
 *
 * This function shall return \c true on success, \c false on failure
 */
bool QCCTV_ReadCommandPacket (QCCTV_CommandPacket* packet,
                              const QByteArray& data)
{
    /* Packet pointer is invalid and/or data incomplete */
    if (!packet || data.isEmpty())
        return false;

    /* Get JSON object from data */
    QJsonObject json = QJsonDocument::fromBinaryData (data).object();
    if (json.isEmpty())
        return false;

    /* Get information from JSON object */
    packet->oldFps = json.value (KEY_OLD_FPS).toInt();
    packet->newFps = json.value (KEY_NEW_FPS).toInt();
    packet->oldZoom = json.value (KEY_OLD_ZOOM).toInt();
    packet->newZoom = json.value (KEY_NEW_ZOOM).toInt();
    packet->focusRequest = json.value (KEY_FOCUS_REQUEST).toBool();
    packet->oldResolution = json.value (KEY_OLD_RESOLUTION).toInt();
    packet->newResolution = json.value (KEY_NEW_RESOLUTION).toInt();
    packet->oldFlashlightEnabled = json.value (KEY_OLD_FLASHLIGHT).toBool();
    packet->newFlashlightEnabled = json.value (KEY_NEW_FLASHLIGHT).toBool();
    packet->oldAutoRegulateResolution = json.value (KEY_OLD_AUTOREGRES).toBool();
    packet->newAutoRegulateResolution = json.value (KEY_NEW_AUTOREGRES).toBool();

    /* Check command flags have changed since last packet */
    packet->fpsChanged = (packet->oldFps != packet->newFps);
    packet->zoomChanged = (packet->oldZoom != packet->newZoom);
    packet->resolutionChanged = (packet->oldResolution != packet->newResolution);
    packet->flashlightEnabledChanged = (packet->oldFlashlightEnabled != packet->newFlashlightEnabled);
    packet->autoRegulateResolutionChanged = (packet->oldAutoRegulateResolution !=
                                             packet->newAutoRegulateResolution);

    /* Packet read successfully */
    return true;
}