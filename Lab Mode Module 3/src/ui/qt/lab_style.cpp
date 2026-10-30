/* lab_style.cpp - Lab Mode styling (see lab_style.h).
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "lab_style.h"
#include "lab_profiles.h"

#include "packet_list.h"    // first: brings in config.h and glib.h, which the epan headers need
#include <epan/color_filters.h>
#include <epan/column.h>
#include <epan/prefs.h>
#include "ui/preference_utils.h"
#include "ui/simple_dialog.h"

#include <QCoreApplication>

// A rule colors matching packets with temporary color "slot" (View > Colorize Conversation > Color 1-10).
struct LabColorRule { guint8 slot; const char *filter; };
struct LabColumn    { const char *title; const char *field; };
struct LabStyleSpec { LabColorRule rules[4]; LabColumn columns[3]; };   // unused entries stay NULL

// Indexed by LabId. Color 1 (light red) marks each attack's key packet, 8 (yellow) the next most
// important, 7/6/4 (green/cyan/blue) normal traffic. If rules overlap, the lower slot wins.
static const LabStyleSpec lab_styles[LAB_COUNT] = {
    { {}, {} },     // LAB_OFF: normal Wireshark
    { { { 1, "tcp.flags.reset == 1" }, { 8, "tcp.port == 23 && tcp.len > 0" },
        { 7, "tcp.flags.syn == 1 && tcp.flags.ack == 0" }, { 6, "tcp.flags.syn == 1 && tcp.flags.ack == 1" } },
      { { "Seq (raw)", "tcp.seq_raw" }, { "Ack (raw)", "tcp.ack_raw" }, { "Flags", "tcp.flags.str" } } },
    { { { 1, "arp.duplicate-address-detected" }, { 8, "arp.opcode == 2" }, { 4, "arp.opcode == 1" } },
      { { "ARP op", "arp.opcode" }, { "Sender MAC", "arp.src.hw_mac" }, { "Sender IP", "arp.src.proto_ipv4" } } },
    { { { 1, "icmp.type == 5" }, { 8, "icmp.type == 3" }, { 7, "icmp.type == 8" }, { 6, "icmp.type == 0" } },
      { { "ICMP type", "icmp.type" }, { "ICMP code", "icmp.code" }, { "Gateway", "icmp.redir_gw" } } },
    { { { 2, "dns.flags.rcode != 0" }, { 8, "dns.flags.response == 1" }, { 4, "dns.flags.response == 0" } },
      { { "DNS ID", "dns.id" }, { "Query", "dns.qry.name" }, { "Rcode", "dns.flags.rcode" } } },
    { { { 1, "ssh" }, { 6, "tls" }, { 8, "icmp" } },
      { { "TCP stream", "tcp.stream" } } },
};

// Show a color filter error the way MainWindow does, then free the message.
static void reportColorError(gchar *err_msg)
{
    simple_dialog(ESD_TYPE_ERROR, ESD_BTN_OK, "%s", err_msg);
    g_free(err_msg);
}

LabStyle::LabStyle(PacketList *pl, QObject *parent) :
    QObject(parent),
    packet_list_(pl),
    lab_(LAB_OFF)
{
    // Lab columns live only in memory; take them out again when Wireshark quits.
    connect(qApp, &QCoreApplication::aboutToQuit, this, [this]() { removeLabColumns(); });
}

void LabStyle::removeLabColumns()
{
    const LabColumn *cols = lab_styles[lab_].columns;
    GList *node = prefs.col_list;
    while (node) {
        GList *next = node->next;   // read it now: removing unlinks this node
        fmt_data *cfmt = (fmt_data *) node->data;
        for (int i = 0; i < 3 && cols[i].title; i++) {
            // Only remove a column with exactly our title and field; the user's columns stay.
            if (cfmt->fmt == COL_CUSTOM && g_strcmp0(cfmt->title, cols[i].title) == 0
                    && g_strcmp0(cfmt->custom_fields, cols[i].field) == 0) {
                column_prefs_remove_link(node);
                g_list_free_1(node);    // remove_link() unlinks the node but does not free it
                break;
            }
        }
        node = next;
    }
}

void LabStyle::setLab(int lab_id)
{
    if (lab_id < 0 || lab_id >= LAB_COUNT)
        return;

    // 1. Undo the previous lab: clear the temporary colors and remove its columns.
    gchar *err_msg = NULL;
    if (!color_filters_reset_tmp(&err_msg))
        reportColorError(err_msg);
    removeLabColumns();

    // 2. Apply the new lab. Position 0 inserts each column just before Info.
    lab_ = lab_id;
    const LabStyleSpec &style = lab_styles[lab_];
    for (int i = 0; i < 4 && style.rules[i].filter; i++) {
        if (!color_filters_set_tmp(style.rules[i].slot, style.rules[i].filter, FALSE, &err_msg))
            reportColorError(err_msg);
    }
    for (int i = 0; i < 3 && style.columns[i].title; i++)
        column_prefs_add_custom(COL_CUSTOM, style.columns[i].title, style.columns[i].field, 0);

    // No prefs_main_write(): lab columns must never be saved to the user's preferences.
    // Both calls mark every row as stale, so rows re-dissect themselves (and fill the
    // new columns) when drawn; no full cf_redissect_packets() is needed.
    packet_list_->columnsChanged();
    packet_list_->recolorPackets();
}
