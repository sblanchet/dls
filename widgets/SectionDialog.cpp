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
#include "Section.h"

using DLS::SectionDialog;

/****************************************************************************/

/** Constructor.
 */
SectionDialog::SectionDialog(
        Section *section,
        QWidget *parent
        ):
    QDialog(parent),
    section(section)
{
    setupUi(this);

    radioButtonAuto->setChecked(section->getAutoScale());
    radioButtonManual->setChecked(!section->getAutoScale());

    QString num;
    num.setNum(section->getScaleMinimum());
    lineEditMinimum->setText(num);
    num.setNum(section->getScaleMaximum());
    lineEditMaximum->setText(num);
}

/****************************************************************************/

/** Destructor.
 */
SectionDialog::~SectionDialog()
{
}

/****************************************************************************/

/** Destructor.
 */
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

    section->setScaleMinimum(min);
    section->setScaleMaximum(max);
    section->setAutoScale(radioButtonAuto->isChecked());

    setResult(Accepted);
    close();
}

/****************************************************************************/
