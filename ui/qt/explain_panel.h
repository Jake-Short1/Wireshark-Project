/* explain_panel.h - Lab Mode: dock that explains the selected packet in plain English.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef EXPLAIN_PANEL_H
#define EXPLAIN_PANEL_H

#include <QDockWidget>

class CaptureFile;
class QLabel;

class ExplainPanel : public QDockWidget
{
    Q_OBJECT
public:
    ExplainPanel(CaptureFile *cf, QWidget *parent);    // parent = main window
public slots:
    void framesSelected(QList<int> rows);               // connected to PacketList::framesSelected
    void setLab(int lab_id);                            // connected to LabToolBar::labChanged
private:
    CaptureFile *cap_file_;
    QLabel *text_;
    int lab_;                   // current LabId
};

#endif // EXPLAIN_PANEL_H
