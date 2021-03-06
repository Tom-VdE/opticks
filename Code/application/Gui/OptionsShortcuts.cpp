/*
 * The information in this file is
 * Copyright(c) 2007 Ball Aerospace & Technologies Corporation
 * and is subject to the terms and conditions of the
 * GNU Lesser General Public License Version 2.1
 * The license text is available from   
 * http://www.gnu.org/licenses/lgpl.html
 */

#include <QtGui/QAction>
#include <QtGui/QApplication>
#include <QtGui/QHeaderView>
#include <QtGui/QImage>
#include <QtGui/QMenu>
#include <QtGui/QPixmap>
#include <QtGui/QTreeWidgetItem>
#include <QtGui/QVBoxLayout>

#include "ApplicationWindow.h"
#include "AppVerify.h"
#include "CustomTreeWidget.h"
#include "DynamicObject.h"
#include "LabeledSection.h"
#include "ObjectResource.h"
#include "OptionsShortcuts.h"
#include "PlugInManagerServicesImp.h"

#include <string>
using namespace std;

OptionsShortcuts::OptionsShortcuts() :
   QWidget(NULL)
{
   // Shortcut tree
   QStringList columnNames;
   columnNames.append("Description");
   columnNames.append("Shortcut");

   mpShortcutTree = new CustomTreeWidget(this);
   mpShortcutTree->setColumnCount(2);
   mpShortcutTree->setHeaderLabels(columnNames);
   mpShortcutTree->setRootIsDecorated(true);
   mpShortcutTree->setSelectionMode(QAbstractItemView::SingleSelection);
   mpShortcutTree->setSortingEnabled(true);
   mpShortcutTree->setGridlinesShown(Qt::Horizontal | Qt::Vertical, true);
   mpShortcutTree->setContextMenuPolicy(Qt::CustomContextMenu);

   QHeaderView* pHeader = mpShortcutTree->header();
   if (pHeader != NULL)
   {
      pHeader->setStretchLastSection(false);
      pHeader->resizeSection(0, 150);
      pHeader->resizeSection(1, 100);
      pHeader->setDefaultAlignment(Qt::AlignLeft | Qt::AlignVCenter);
      pHeader->setSortIndicatorShown(true);
   }

   LabeledSection* pShortcutSection = new LabeledSection(mpShortcutTree, "Current Shortcuts", this);

   // Dialog layout
   QVBoxLayout* pLayout = new QVBoxLayout(this);
   pLayout->setMargin(0);
   pLayout->setSpacing(10);
   pLayout->addWidget(pShortcutSection);

   // Connections
   VERIFYNR(connect(mpShortcutTree, SIGNAL(cellTextChanged(QTreeWidgetItem*, int)), this,
      SLOT(shortcutChanged(QTreeWidgetItem*, int))));
   VERIFYNR(connect(mpShortcutTree, SIGNAL(customContextMenuRequested(const QPoint&)), this,
      SLOT(displayContextMenu(const QPoint&))));

   // Initialize From Settings
   shortcutsIntoGui(ApplicationWindow::getSettingShortcuts(), NULL);
   mpShortcutTree->sortItems(0, Qt::AscendingOrder);
   updateDuplicateIndicators();
}

OptionsShortcuts::~OptionsShortcuts()
{}

void OptionsShortcuts::applyChanges()
{
   FactoryResource<DynamicObject> pShortcutRes;

   for (int count = 0; count < mpShortcutTree->topLevelItemCount(); count++)
   {
      QTreeWidgetItem* pItem = mpShortcutTree->topLevelItem(count);
      if (pItem->text(0).toStdString() == "Plug-Ins")
      {
         for (int plugInCount = 0; plugInCount < pItem->childCount(); plugInCount++)
         {
            shortcutsFromGui(pItem->child(plugInCount), pShortcutRes.get());
         }
      }
      else
      {
         shortcutsFromGui(pItem, pShortcutRes.get());
      }
   }
   ApplicationWindow::setSettingShortcuts(pShortcutRes.get());

   //Now update all found QAction's to have the new shortcut value
   const DynamicObject* pShortcuts = ApplicationWindow::getSettingShortcuts();
   if (pShortcuts != NULL)
   {
      // Update the actions with the current shortcut values
      QWidgetList topWidgets = QApplication::topLevelWidgets();
      QWidgetList::iterator widgetIter;
      for (widgetIter = topWidgets.begin(); widgetIter != topWidgets.end(); ++widgetIter)
      {
         QList<QAction*> actions = (*widgetIter)->findChildren<QAction*>();

         for (int i = 0; i < actions.count(); ++i)
         {
            QAction* pAction = actions[i];
            if (pAction != NULL)
            {
               QMap<QString, QVariant> actionData = pAction->data().toMap();

               QMap<QString, QVariant>::const_iterator iter = actionData.find("ShortcutContext");
               if (iter != actionData.end())
               {
                  string strContext = iter.value().toString().toStdString();
                  string strName = pAction->toolTip().toStdString();

                  const DataVariant& dv = pShortcuts->getAttributeByPath(strContext + "/" + strName);
                  if (dv.isValid() && dv.getTypeName() == "string")
                  {
                     QString keySequence = QString::fromStdString(dv_cast<string>(dv));
                     pAction->setShortcut(QKeySequence(keySequence));
                  }
               }
            }
         }
      }
   }
}

QString OptionsShortcuts::getShortcut(QTreeWidgetItem* pItem) const
{
   if (pItem == NULL)
   {
      return QString();
   }

   QString shortcut = pItem->text(1);
   if (shortcut.isEmpty() == false)
   {
      while (pItem != NULL)
      {
         QString shortcutContext = pItem->text(0);
         if (shortcutContext.isEmpty() == false)
         {
            shortcut.prepend(shortcutContext + "/");
         }

         pItem = pItem->parent();
      }
   }

   return shortcut;
}

QTreeWidgetItem* OptionsShortcuts::getShortcutItem(const QString& shortcut) const
{
   if (shortcut.isEmpty() == true)
   {
      return NULL;
   }

   QTreeWidgetItemIterator iter(mpShortcutTree);
   while (*iter != NULL)
   {
      QTreeWidgetItem* pItem = *iter;
      if (pItem != NULL)
      {
         QString currentShortcut = getShortcut(pItem);
         if (currentShortcut == shortcut)
         {
            return pItem;
         }
      }

      ++iter;
   }

   return NULL;
}

QList<QTreeWidgetItem*> OptionsShortcuts::getDuplicates(QTreeWidgetItem* pItem) const
{
   QList<QTreeWidgetItem*> duplicateItems;
   if (pItem != NULL)
   {
      QString shortcutText = pItem->text(1);
      if (shortcutText.isEmpty() == false)
      {
         QTreeWidgetItemIterator iter(mpShortcutTree);
         while (*iter != NULL)
         {
            QTreeWidgetItem* pCurrentItem = *iter;
            if ((pCurrentItem != pItem) && (pCurrentItem != NULL) && (pCurrentItem->text(1) == shortcutText))
            {
               duplicateItems.append(pCurrentItem);
            }

            ++iter;
         }
      }
   }

   return duplicateItems;
}

void OptionsShortcuts::updateDuplicateIndicators()
{
   for (int i = 0; i < mpShortcutTree->topLevelItemCount(); ++i)
   {
      QTreeWidgetItem* pTopItem = mpShortcutTree->topLevelItem(i);
      if (pTopItem != NULL)
      {
         updateDuplicateIndicator(pTopItem);
      }
   }
}

QIcon OptionsShortcuts::updateDuplicateIndicator(QTreeWidgetItem* pItem)
{
   if (pItem == NULL)
   {
      return QIcon();
   }

   QIcon warningIcon(":/icons/Warning");
   QIcon okIcon(":/icons/Ok");

   QIcon duplicateIcon;
   int column = 0;

   int numChildren = pItem->childCount();
   if (numChildren == 0)
   {
      column = 1;

      QString shortcut = pItem->text(column);
      if (shortcut.isEmpty() == false)
      {
         QList<QTreeWidgetItem*> duplicateItems = getDuplicates(pItem);
         if (duplicateItems.empty() == true)
         {
            duplicateIcon = okIcon;
         }
         else
         {
            duplicateIcon = warningIcon;
         }
      }
   }
   else
   {
      duplicateIcon = okIcon;

      QPixmap warningPix = warningIcon.pixmap(16, 16);
      QImage warningImage = warningPix.toImage();

      for (int i = 0; i < pItem->childCount(); ++i)
      {
         QTreeWidgetItem* pChildItem = pItem->child(i);
         if (pChildItem != NULL)
         {
            QIcon childIcon = updateDuplicateIndicator(pChildItem);
            if (childIcon.isNull() == false)
            {
               QPixmap childPix = childIcon.pixmap(16, 16);
               QImage childImage = childPix.toImage();
               if (childImage == warningImage)
               {
                  duplicateIcon = warningIcon;
               }
            }
         }
      }
   }

   pItem->setIcon(column, duplicateIcon);
   return duplicateIcon;
}

void OptionsShortcuts::shortcutsIntoGui(const DynamicObject* pCurObject, QTreeWidgetItem* pParent)
{
   Service<PlugInManagerServices> pManager;
   vector<string> attributes;
   vector<string>::iterator attrIter;
   pCurObject->getAttributeNames(attributes);

   QTreeWidgetItem* pPlugInItem = NULL;
   for (attrIter = attributes.begin(); attrIter != attributes.end(); ++attrIter)
   {
      string name = *attrIter;
      const DataVariant& dv = pCurObject->getAttribute(*attrIter);
      if (dv.isValid())
      {
         QTreeWidgetItem* pItem = NULL;
         if (pParent == NULL)
         {
            //see if this item is a plug-in, if so put under a "PlugIns" item in the tree widget
            if (pManager->getPlugInDescriptor(name) != NULL)
            {
               if (pPlugInItem == NULL)
               {
                  pPlugInItem = new QTreeWidgetItem(mpShortcutTree);
                  pPlugInItem->setText(0, "Plug-Ins");
                  pPlugInItem->setBackgroundColor(1, QColor(235, 235, 235));
               }
               pItem = new QTreeWidgetItem(pPlugInItem);
            }
            else
            {
               pItem = new QTreeWidgetItem(mpShortcutTree);
            }
         }
         else
         {
            pItem = new QTreeWidgetItem(pParent);
         }

         if (dv.getTypeName() == "DynamicObject")
         {
            const DynamicObject* pObject = dv_cast<DynamicObject>(&dv);
            pItem->setText(0, QString::fromStdString(name));
            pItem->setBackgroundColor(1, QColor(235, 235, 235));
            shortcutsIntoGui(pObject, pItem);
         }
         else if (dv.getTypeName() == "string")
         {
            QString shortcut = QString::fromStdString(dv_cast<string>(dv));
            mpShortcutTree->setCellWidgetType(pItem, 1, CustomTreeWidget::SHORTCUT_EDIT);
            pItem->setBackgroundColor(1, Qt::white);
            pItem->setText(0, QString::fromStdString(name));
            pItem->setText(1, shortcut);
         }
      }
   }
}

void OptionsShortcuts::shortcutsFromGui(QTreeWidgetItem* pCurObject, DynamicObject* pParent) const
{
   if (pCurObject->childCount() != 0)
   {
      FactoryResource<DynamicObject> pNewObject;
      string name = pCurObject->text(0).toStdString();
      pParent->setAttribute(name, *pNewObject);
      DynamicObject* pObject = dv_cast<DynamicObject>(&pParent->getAttribute(name));
      if (pObject != NULL)
      {
         for (int count = 0; count < pCurObject->childCount(); count++)
         {
            QTreeWidgetItem* pItem = pCurObject->child(count);
            shortcutsFromGui(pItem, pObject);
         }
      }
   }
   else
   {
      string name = pCurObject->text(0).toStdString();
      string shortcut = pCurObject->text(1).toStdString();
      pParent->setAttribute(name, shortcut);
   }
}

void OptionsShortcuts::shortcutChanged(QTreeWidgetItem* pItem, int column)
{
   if ((pItem != NULL) && (column == 1))
   {
      updateDuplicateIndicators();
   }
}

void OptionsShortcuts::displayContextMenu(const QPoint& pos)
{
   QMenu contextMenu(mpShortcutTree);

   QList<QTreeWidgetItem*> items = mpShortcutTree->selectedItems();
   if (items.count() == 1)
   {
      QTreeWidgetItem* pItem  = items.front();
      VERIFYNRV(pItem != NULL);

      // Display a menu to activate duplicate items if the selected item is currently visible
      QRect itemRect = mpShortcutTree->visualItemRect(pItem);
      QRect treeRect = mpShortcutTree->rect();
      bool visible = treeRect.intersects(itemRect);

      QString shortcut = pItem->text(1);
      if ((shortcut.isEmpty() == false) && (visible == true))
      {
         QList<QTreeWidgetItem*> duplicateItems = getDuplicates(pItem);
         if (duplicateItems.empty() == false)
         {
            QMenu* pDuplicatesMenu = contextMenu.addMenu("Duplicates");
            if (pDuplicatesMenu != NULL)
            {
               for (int i = 0; i < duplicateItems.count(); ++i)
               {
                  QTreeWidgetItem* pDuplicateItem = duplicateItems[i];
                  if (pDuplicateItem != NULL)
                  {
                     QString shortcut = getShortcut(pDuplicateItem);
                     if (shortcut.isEmpty() == false)
                     {
                        pDuplicatesMenu->addAction(shortcut);
                     }
                  }
               }

               VERIFYNRV(connect(pDuplicatesMenu, SIGNAL(triggered(QAction*)), this,
                  SLOT(activateDuplicate(QAction*))));
               contextMenu.addSeparator();
            }
         }
      }
   }

   contextMenu.addAction("Expand All", mpShortcutTree, SLOT(expandAll()));
   contextMenu.addAction("Collapse All", mpShortcutTree, SLOT(collapseAll()));
   contextMenu.exec(mpShortcutTree->viewport()->mapToGlobal(pos));
}

void OptionsShortcuts::activateDuplicate(QAction* pAction)
{
   if (pAction == NULL)
   {
      return;
   }

   QString shortcut = pAction->text();
   if (shortcut.isEmpty() == false)
   {
      QTreeWidgetItem* pItem = getShortcutItem(shortcut);
      if (pItem != NULL)
      {
         mpShortcutTree->scrollToItem(pItem);
         mpShortcutTree->setCurrentItem(pItem);
      }
   }
}
