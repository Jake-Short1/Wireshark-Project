/* field_copy_panel.h - Lab Mode: dock listing the selected packet's key fields, each with a Copy button.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef FIELD_COPY_PANEL_H
#define FIELD_COPY_PANEL_H

#include <QDockWidget>

class CaptureFile;
class QScrollArea;

class FieldCopyPanel : public QDockWidget
{
    Q_OBJECT
public:
    FieldCopyPanel(CaptureFile *cf, QWidget *parent);   // parent = main window

public slots:
    void framesSelected(QList<int> rows);               // connected to PacketList::framesSelected

private:
    CaptureFile *cap_file_;
    QScrollArea *scroll_area_;  // shows the current rows; its page is replaced on every selection
};

#endif // FIELD_COPY_PANEL_H
