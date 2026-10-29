/* field_copy_panel.cpp - Lab Mode Field Copy panel (see field_copy_panel.h).
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "field_copy_panel.h"

#include <epan/epan_dissect.h>
#include <epan/proto.h>
#include "capture_file.h"

#include <QApplication>
#include <QClipboard>
#include <QFontDatabase>
#include <QGridLayout>
#include <QLabel>
#include <QPointer>
#include <QPushButton>
#include <QScrollArea>
#include <QTimer>

// Values students type into attack scripts, in display order. TCP seq/ack are raw, not relative.
struct FieldSpec { const char *label; const char *abbrev; };
static const FieldSpec field_specs[] = {
    { "Source MAC", "eth.src" },             { "Destination MAC", "eth.dst" },
    { "Source IP", "ip.src" },               { "Destination IP", "ip.dst" },
    { "IP ID", "ip.id" },                    { "IP TTL", "ip.ttl" },
    { "TCP source port", "tcp.srcport" },    { "TCP destination port", "tcp.dstport" },
    { "TCP seq (raw)", "tcp.seq_raw" },      { "TCP ack (raw)", "tcp.ack_raw" },
    { "TCP payload length", "tcp.len" },     { "TCP flags", "tcp.flags.str" },  { "TCP window", "tcp.window_size_value" },
    { "UDP source port", "udp.srcport" },    { "UDP destination port", "udp.dstport" },
    { "ICMP type", "icmp.type" },            { "ICMP code", "icmp.code" },      { "ICMP gateway", "icmp.redir_gw" },
    { "ARP opcode", "arp.opcode" },          { "ARP sender MAC", "arp.src.hw_mac" },  { "ARP sender IP", "arp.src.proto_ipv4" },
    { "ARP target MAC", "arp.dst.hw_mac" },  { "ARP target IP", "arp.dst.proto_ipv4" },
    { "DNS transaction ID", "dns.id" },      { "DNS query name", "dns.qry.name" },
};

// Display text of the first (outermost) occurrence of a field, or "" if the packet lacks it.
static QString fieldText(epan_dissect_t *edt, const char *abbrev)
{
    // Search the whole tree; proto_get_finfo_ptr_array() only sees fields a filter or tap asked for.
    GPtrArray *found = proto_find_first_finfo(edt->tree, proto_registrar_get_id_byname(abbrev));
    QString text;
    if (found->len > 0) {
        field_info *fi = (field_info *) g_ptr_array_index(found, 0);
        gchar *s = fvalue_to_string_repr(NULL, &fi->value, FTREPR_DISPLAY, fi->hfinfo->display);
        text = QString::fromUtf8(s);
        wmem_free(NULL, s);
    }
    g_ptr_array_free(found, TRUE);
    return text;
}

// A button that copies text to the clipboard and says "Copied!" for one second.
static QPushButton *copyButton(const QString &text)
{
    // A QPointer becomes NULL if the rows are rebuilt (deleting the button) before the timer fires.
    QPointer<QPushButton> button = new QPushButton(FieldCopyPanel::tr("Copy"));
    QObject::connect(button, &QPushButton::clicked, [button, text]() {
        QApplication::clipboard()->setText(text);
        button->setText(FieldCopyPanel::tr("Copied!"));
        QTimer::singleShot(1000, [button]() {
            if (button)
                button->setText(FieldCopyPanel::tr("Copy"));
        });
    });
    return button;
}

FieldCopyPanel::FieldCopyPanel(CaptureFile *cf, QWidget *parent) :
    QDockWidget(tr("Lab Fields (Copy)"), parent),
    cap_file_(cf),
    scroll_area_(new QScrollArea(this))
{
    scroll_area_->setWidgetResizable(true);
    scroll_area_->setMinimumWidth(fontMetrics().averageCharWidth() * 60);   // room for label, value and button
    setWidget(scroll_area_);
    framesSelected(QList<int>());   // start with the hint
    // Closing a file does not emit framesSelected(), so show the hint again here.
    connect(cf, &CaptureFile::captureEvent, this, [this](CaptureEvent e) {
        if (e.eventType() == CaptureEvent::Closed)
            framesSelected(QList<int>());
    });
}

void FieldCopyPanel::framesSelected(QList<int> rows)
{
    // A fresh page each time; QScrollArea::setWidget() deletes the old page and all its rows.
    QWidget *page = new QWidget;
    QGridLayout *grid = new QGridLayout(page);
    capture_file *cf = cap_file_->capFile();
    int row = 0;

    // A deselect also arrives as one row, so also check that a dissection exists.
    if (rows.size() == 1 && cf && cf->edt) {
        for (size_t i = 0; i < G_N_ELEMENTS(field_specs); i++) {
            QString value = fieldText(cf->edt, field_specs[i].abbrev);
            if (value.isEmpty())
                continue;   // show only the fields this packet has
            QLabel *value_label = new QLabel(value);
            value_label->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
            value_label->setTextInteractionFlags(Qt::TextSelectableByMouse);
            grid->addWidget(new QLabel(field_specs[i].label), row, 0);
            grid->addWidget(value_label, row, 1);
            grid->addWidget(copyButton(value), row, 2);
            row++;
        }
    }

    if (row == 0)
        grid->addWidget(new QLabel(tr("Select one packet to see its key fields.")), row++, 0, 1, 3);
    grid->setRowStretch(row, 1);    // keep the rows at the top
    scroll_area_->setWidget(page);
}
