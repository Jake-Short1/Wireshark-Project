/* lab_toolbar.h - Lab Mode toolbar: pick a course lab, then apply its preset display filters.
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#ifndef LAB_TOOLBAR_H
#define LAB_TOOLBAR_H

#include "lab_profiles.h"

#include <QComboBox>
#include <QList>
#include <QToolBar>

class LabToolBar : public QToolBar
{
    Q_OBJECT
public:
    explicit LabToolBar(QWidget *parent = 0);
    LabId currentLab() const { return static_cast<LabId>(lab_combo_->currentIndex()); }

signals:
    void labChanged(int lab_id);              // emitted on every lab selection, including LAB_OFF
    void filterRequested(QString filter, bool prepare_only);  // "" means "clear the display filter"

private slots:
    void labActivated(int index);

private:
    QComboBox *lab_combo_;
    QList<QAction *> preset_actions_;  // the current lab's preset buttons
};

#endif // LAB_TOOLBAR_H
