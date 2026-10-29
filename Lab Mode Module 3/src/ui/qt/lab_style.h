/* lab_style.h - Lab Mode: lab-specific packet colors and extra packet list columns.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LAB_STYLE_H
#define LAB_STYLE_H

#include <QObject>

class PacketList;

class LabStyle : public QObject
{
    Q_OBJECT
public:
    LabStyle(PacketList *pl, QObject *parent);

public slots:
    void setLab(int lab_id);    // connected to LabToolBar::labChanged

private:
    void removeLabColumns();    // removes the columns that lab_ added

    PacketList *packet_list_;
    int lab_;                  // lab whose styling is applied now (LAB_OFF = none)
};

#endif // LAB_STYLE_H
