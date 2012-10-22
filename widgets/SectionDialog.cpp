/*****************************************************************************
 *
 * $Id$
 *
 * Copyright (C) 2012  Florian Pose <fp@igh-essen.com>
 *
 * This file is part of the DLS widget library.
 *
 * The DLS widget library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the License,
 * or (at your option) any later version.
 *
 * The DLS widget library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the DLS widget library. If not, see
 * <http://www.gnu.org/licenses/>.
 *
 ****************************************************************************/

#include "SectionDialog.h"
#include "SectionModel.h"

using DLS::SectionDialog;

/****************************************************************************/

/** Constructor.
 */
SectionDialog::SectionDialog(
        Section *section,
        QWidget *parent
        ):
    QDialog(parent),
    section(section),
    origSection(*section),
    workSection(*section),
    model(new SectionModel(&workSection)),
    colorDelegate(this)
{
    setupUi(this);

    radioButtonAuto->setChecked(section->getAutoScale());
    radioButtonManual->setChecked(!section->getAutoScale());

    QString num;
    num.setNum(section->getScaleMinimum());
    lineEditMinimum->setText(num);
    num.setNum(section->getScaleMaximum());
    lineEditMaximum->setText(num);

    connect(model,
            SIGNAL(dataChanged(const QModelIndex &, const QModelIndex &)),
            this, SLOT(modelDataChanged()));

    tableViewLayers->setItemDelegateForColumn(3, &colorDelegate);
    tableViewLayers->setModel(model);
    tableViewLayers->verticalHeader()->hide();
    QHeaderView *header = tableViewLayers->horizontalHeader();
    header->setResizeMode(0, QHeaderView::Stretch);
    header->setResizeMode(1, QHeaderView::Stretch);
    header->setResizeMode(2, QHeaderView::ResizeToContents);
    header->setResizeMode(3, QHeaderView::ResizeToContents);
    header->setResizeMode(4, QHeaderView::ResizeToContents);
    header->setResizeMode(5, QHeaderView::ResizeToContents);
    tableViewLayers->resizeColumnsToContents();

    connect(radioButtonAuto, SIGNAL(toggled(bool)),
            this, SLOT(scaleValueChanged()));
    connect(radioButtonManual, SIGNAL(toggled(bool)),
            this, SLOT(scaleValueChanged()));
    connect(lineEditMinimum, SIGNAL(textChanged(const QString &)),
            this, SLOT(scaleValueChanged()));
    connect(lineEditMaximum, SIGNAL(textChanged(const QString &)),
            this, SLOT(scaleValueChanged()));
}

/****************************************************************************/

/** Destructor.
 */
SectionDialog::~SectionDialog()
{
    delete model;
}

/****************************************************************************/

void SectionDialog::accept()
{
    bool ok;
    double min, max;

    min = lineEditMinimum->text().toDouble(&ok);
    if (!ok) {
        return;
    }

    max = lineEditMaximum->text().toDouble(&ok);
    if (!ok) {
        return;
    }

    workSection.setScaleMinimum(min);
    workSection.setScaleMaximum(max);
    workSection.setAutoScale(radioButtonAuto->isChecked());

    *section = workSection;

    done(Accepted);
}

/****************************************************************************/

void SectionDialog::reject()
{
    *section = origSection;
    done(Rejected);
}

/****************************************************************************/

void SectionDialog::scaleValueChanged()
{
    bool ok;
    double min, max;

    min = lineEditMinimum->text().toDouble(&ok);
    if (ok) {
        workSection.setScaleMinimum(min);
    }

    max = lineEditMaximum->text().toDouble(&ok);
    if (ok) {
        workSection.setScaleMaximum(max);
    }

    workSection.setAutoScale(radioButtonAuto->isChecked());

    if (checkBoxPreview->isChecked()) {
        *section = workSection;
    }
}

/****************************************************************************/

void SectionDialog::on_checkBoxPreview_toggled()
{
    if (checkBoxPreview->isChecked()) {
        *section = workSection;
    }
    else {
        *section = origSection;
    }
}

/****************************************************************************/

void SectionDialog::modelDataChanged()
{
    if (checkBoxPreview->isChecked()) {
        *section = workSection;
    }
}

/****************************************************************************/
