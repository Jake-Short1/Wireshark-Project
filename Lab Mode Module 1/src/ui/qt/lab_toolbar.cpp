/* lab_toolbar.cpp - Lab Mode toolbar (see lab_toolbar.h).
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "lab_toolbar.h"

#include <QAction>
#include <QApplication>
#include <QLabel>

LabToolBar::LabToolBar(QWidget *parent) :
    QToolBar(parent)
{
    addWidget(new QLabel(tr("Lab:")));

    // One entry per lab in LabId order, so the combo index is the LabId.
    lab_combo_ = new QComboBox(this);
    for (int i = 0; i < LAB_COUNT; i++) {
        lab_combo_->addItem(lab_profiles[i].title);
        lab_combo_->setItemData(i, lab_profiles[i].hint, Qt::ToolTipRole);
    }
    addWidget(lab_combo_);

    // activated() fires only for user picks (so startup keeps the filter); SIGNAL() avoids QOverload (C++14).
    connect(lab_combo_, SIGNAL(activated(int)), this, SLOT(labActivated(int)));
}

void LabToolBar::labActivated(int index)
{
    const LabProfile &lab = lab_profiles[index];

    // Delete the previous lab's buttons; deleting a QAction also removes it from the toolbar.
    qDeleteAll(preset_actions_);
    preset_actions_.clear();

    for (int i = 0; i < lab.n_presets; i++) {
        const LabPreset &preset = lab.presets[i];
        QAction *action = addAction(preset.label);
        action->setToolTip(QString("%1\n%2").arg(preset.tip, tr("Shift+click to edit before applying.")));
        // The lambda copies the preset. Shift+click only prepares the filter, as in Wireshark's filter buttons.
        connect(action, &QAction::triggered, this, [this, preset]() {
            emit filterRequested(preset.filter, QApplication::keyboardModifiers() & Qt::ShiftModifier);
        });
        preset_actions_ << action;
    }

    emit filterRequested(lab.base_filter, false);  // "" for LAB_OFF clears the filter
    emit labChanged(lab.id);
}
