/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the Qt Assistant of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "optionswidget.h"

#include <QtWidgets/QComboBox>
#include <QtWidgets/QItemDelegate>
#include <QtWidgets/QListWidget>
#include <QtWidgets/QVBoxLayout>

QT_BEGIN_NAMESPACE

class ListWidgetDelegate : public QItemDelegate
{
//    Q_OBJECT not needed
public:
    ListWidgetDelegate(QWidget *w) : QItemDelegate(w), m_widget(w) {}

    static bool isSeparator(const QModelIndex &index) {
        return index.data(Qt::AccessibleDescriptionRole).toString() == QLatin1String("separator");
    }
    static void setSeparator(QListWidgetItem *item) {
        item->setData(Qt::AccessibleDescriptionRole, QString::fromLatin1("separator"));
        item->setFlags(item->flags() & ~(Qt::ItemIsSelectable|Qt::ItemIsEnabled));
    }

protected:
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const override {
        if (isSeparator(index)) {
            QRect rect = option.rect;
            if (const QAbstractItemView *view = qobject_cast<const QAbstractItemView*>(option.widget))
                rect.setWidth(view->viewport()->width());
            QStyleOption opt;
            opt.rect = rect;
            m_widget->style()->drawPrimitive(QStyle::PE_IndicatorToolBarSeparator, &opt, painter, m_widget);
        } else {
            QItemDelegate::paint(painter, option, index);
        }
    }

    QSize sizeHint(const QStyleOptionViewItem &option,
                   const QModelIndex &index) const override {
        if (isSeparator(index)) {
            int pm = m_widget->style()->pixelMetric(QStyle::PM_DefaultFrameWidth, nullptr, m_widget);
            return QSize(pm, pm);
        }
        return QItemDelegate::sizeHint(option, index);
    }
private:
    QWidget *m_widget;
};

static QStringList subtract(const QStringList &minuend, const QStringList &subtrahend)
{
    QStringList result = minuend;
    for (const QString &str : subtrahend)
        result.removeOne(str);
    return result;
}

/////////////////

OptionsWidget::OptionsWidget(QWidget *parent)
    : QWidget(parent)
{
    m_listWidget = new QListWidget(this);
    m_listWidget->setItemDelegate(new ListWidgetDelegate(m_listWidget));
    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_listWidget);
    layout->setMargin(0);

    connect(m_listWidget, &QListWidget::itemChanged, this, &OptionsWidget::itemChanged);
}

void OptionsWidget::clear()
{
    setOptions(QStringList(), QStringList());
}

void OptionsWidget::setOptions(const QStringList &validOptions,
                               const QStringList &selectedOptions)
{
    m_listWidget->clear();
    m_optionToItem.clear();
    m_itemToOption.clear();

    m_validOptions = validOptions;
    m_validOptions.removeDuplicates();
    qSort(m_validOptions);

    m_selectedOptions = selectedOptions;
    m_selectedOptions.removeDuplicates();
    qSort(m_selectedOptions);

    m_invalidOptions = subtract(m_selectedOptions, m_validOptions);
    const QStringList validSelectedOptions = subtract(m_selectedOptions, m_invalidOptions);
    const QStringList validUnselectedOptions = subtract(m_validOptions, m_selectedOptions);

    for (const QString &option : validSelectedOptions)
        appendItem(option, true, true);

    for (const QString &option : m_invalidOptions)
        appendItem(option, false, true);

    if ((validSelectedOptions.count() + m_invalidOptions.count())
            && validUnselectedOptions.count()) {
        appendSeparator();
    }

    for (const QString &option : validUnselectedOptions) {
        // TODO: make No Option text customizable
        appendItem(option, true, false);
        if (option.isEmpty() && validUnselectedOptions.count() > 1) // special No Option item
            appendSeparator();
    }
}

QStringList OptionsWidget::validOptions() const
{
    return m_validOptions;
}

QStringList OptionsWidget::selectedOptions() const
{
    return m_selectedOptions;
}

QListWidgetItem *OptionsWidget::appendItem(const QString &optionName, bool valid, bool selected)
{
    QString optionText = optionName;
    if (optionName.isEmpty())
        optionText = QLatin1Char('[') + tr("No Option") + QLatin1Char(']');
    if (!valid)
        optionText += QLatin1String("\t[") + tr("Invalid Option") + QLatin1Char(']');
    QListWidgetItem *optionItem = new QListWidgetItem(optionText, m_listWidget);
    optionItem->setCheckState(selected ? Qt::Checked : Qt::Unchecked);
    m_listWidget->insertItem(m_listWidget->count(), optionItem);
    m_optionToItem[optionName] = optionItem;
    m_itemToOption[optionItem] = optionName;
    return optionItem;
}

void OptionsWidget::appendSeparator()
{
    QListWidgetItem *separatorItem = new QListWidgetItem(m_listWidget);
    ListWidgetDelegate::setSeparator(separatorItem);
    m_listWidget->insertItem(m_listWidget->count(), separatorItem);
}

void OptionsWidget::itemChanged(QListWidgetItem *item)
{
    const auto it = m_itemToOption.constFind(item);
    if (it == m_itemToOption.constEnd())
        return;

    const QString option = *it;

    if (item->checkState() == Qt::Checked && !m_selectedOptions.contains(option)) {
        m_selectedOptions.append(option);
        qSort(m_selectedOptions);
    } else if (item->checkState() == Qt::Unchecked && m_selectedOptions.contains(option)) {
        m_selectedOptions.removeOne(option);
    } else {
        return;
    }

    emit optionSelectionChanged(m_selectedOptions);
}


QT_END_NAMESPACE